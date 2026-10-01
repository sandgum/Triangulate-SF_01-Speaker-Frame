#include "audio_out.h"
#include "board.h"
#include "dsp.h"
#include "chime.h"

#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/ringbuf.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "driver/i2s_std.h"
#include "esp_log.h"
#include "esp_check.h"

static const char *TAG = "audio_out";

/* 256 frames = 5.8 ms at 44.1 kHz. */
#define BLOCK_FRAMES   256
#define BLOCK_BYTES    (BLOCK_FRAMES * 2 * sizeof(int16_t))

/* ~185 ms of slack to absorb A2DP burstiness without eating much DRAM. */
#define RING_BYTES     32768

#define DMA_DESC_NUM   6
#define DMA_FRAME_NUM  240

/*
 * Power-up timing, all from the datasheets:
 *
 *  PCM5102A (SLASE99):
 *    - SCK pin 12 is tied to the ESP32 rather than GND, and the classic
 *      ESP32 can only emit a master clock on GPIO 0/1/3. We therefore hold
 *      both SCK pins LOW, which is TI's documented "SCK to GND" mode: the
 *      device runs its internal PLL off BCK (sec 9.4.1). A *floating* SCK
 *      would never lock, so this is not optional.
 *    - The PLL needs valid LRCK/BCK to lock; the part resynchronises if the
 *      LRCK:BCK relationship is invalid for more than 4 LRCK periods
 *      (sec 9.3.2). 20 ms is ~880 LRCK periods at 44.1 kHz — ample.
 *    - XSMT soft mute / un-mute ramps over 104 samples in 1 dB steps
 *      (sec 9.3.3). That is 2.36 ms at 44.1 kHz; we compute it from fs.
 *    - XSMT needs tr/tf < 20 ns (sec 8.7), so it must be driven by a plain
 *      GPIO edge. Do not put an RC on this net: a 1->0 edge slower than
 *      6 ms puts the part into external-undervoltage mode instead.
 *
 *  TPA3116D2 / TPA3130D2 (SLOS708G):
 *    - MUTE high = outputs Hi-Z, low = outputs enabled. Turn-off is ~2 us.
 *    - SDZ is strapped to FAULTZ on this board, so the amps enable
 *      themselves when PVCC comes up; only MUTE is under our control.
 */
#define PLL_LOCK_MS        20
#define XSMT_RAMP_SAMPLES  104
#define DAC_SETTLE_MS      10   /* charge pump + analog settle margin */
#define AMP_SETTLE_MS       5

static i2s_chan_handle_t s_wfr, s_twt;
static RingbufHandle_t   s_rb;
static TaskHandle_t      s_task;
static QueueHandle_t     s_seq_q;
static volatile bool     s_unmuted;
static uint32_t          s_rate = 44100;

static int16_t s_in[BLOCK_FRAMES * 2];
static int16_t s_w [BLOCK_FRAMES * 2];
static int16_t s_t [BLOCK_FRAMES * 2];

static inline uint32_t ramp_ms(void)
{
    uint32_t fs = s_rate ? s_rate : 44100;
    return (XSMT_RAMP_SAMPLES * 1000U + fs - 1U) / fs;   /* ceil */
}

typedef enum { SEQ_MUTE, SEQ_UNMUTE, SEQ_RATE, SEQ_CHIME } seq_op_t;

typedef struct {
    seq_op_t op;
    uint32_t arg;
} seq_cmd_t;

static void seq_task(void *arg);

/* ------------------------------------------------------------ safe state */

void audio_out_safe_state(void)
{
    gpio_config_t io = {
        .pin_bit_mask = (1ULL << PIN_DRIVER_MUTE) | (1ULL << PIN_DAC_EN)
                      | (1ULL << PIN_WFR_SCK)     | (1ULL << PIN_TWT_SCK),
        .mode         = GPIO_MODE_OUTPUT,
        .pull_up_en   = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_DISABLE,
    };
    gpio_config(&io);

    gpio_set_level(PIN_DRIVER_MUTE, AMPS_MUTED);  /* amps Hi-Z first */
    gpio_set_level(PIN_DAC_EN,      DAC_MUTED);   /* XSMT low        */

    /* SCK to GND -> PCM5102A uses its internal PLL off BCK. */
    gpio_set_level(PIN_WFR_SCK, 0);
    gpio_set_level(PIN_TWT_SCK, 0);

    s_unmuted = false;
    ESP_LOGI(TAG, "safe state: amps muted, DACs muted, SCK held low");
}

/* -------------------------------------------------------------- muting */

static void do_unmute(void)
{
    if (s_unmuted) return;

    /* DAC comes up first so the amps never see an un-ramped output. */
    gpio_set_level(PIN_DAC_EN, DAC_UNMUTED);
    vTaskDelay(pdMS_TO_TICKS(ramp_ms() + DAC_SETTLE_MS));

    gpio_set_level(PIN_DRIVER_MUTE, AMPS_UNMUTED);
    s_unmuted = true;
    ESP_LOGI(TAG, "un-muted (DAC ramp %lu ms, then amps)",
             (unsigned long)ramp_ms());
}

static void do_mute(void)
{
    if (!s_unmuted) return;

    /* Amps go Hi-Z first (~2 us) so the DAC's mute ramp is never audible. */
    gpio_set_level(PIN_DRIVER_MUTE, AMPS_MUTED);
    s_unmuted = false;
    vTaskDelay(pdMS_TO_TICKS(AMP_SETTLE_MS));

    gpio_set_level(PIN_DAC_EN, DAC_MUTED);
    vTaskDelay(pdMS_TO_TICKS(ramp_ms()));
    ESP_LOGI(TAG, "muted (amps first, then DAC ramp)");
}

bool audio_out_is_unmuted(void) { return s_unmuted; }

/* ---------------------------------------------------------------- i2s */

static esp_err_t make_channel(i2s_port_t port, i2s_chan_handle_t *out,
                              int bclk, int ws, int dout, uint32_t rate,
                              bool use_apll)
{
    i2s_chan_config_t cc = I2S_CHANNEL_DEFAULT_CONFIG(port, I2S_ROLE_MASTER);
    cc.dma_desc_num  = DMA_DESC_NUM;
    cc.dma_frame_num = DMA_FRAME_NUM;
    cc.auto_clear    = true;          /* emit silence, not stale DMA, on underrun */

    ESP_RETURN_ON_ERROR(i2s_new_channel(&cc, out, NULL), TAG, "new_channel");

    i2s_std_config_t std = {
        .clk_cfg  = I2S_STD_CLK_DEFAULT_CONFIG(rate),
        .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(
                        I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_STEREO),
        .gpio_cfg = {
            /* MCLK unused: SCK is held low by hand, see audio_out_safe_state. */
            .mclk = I2S_GPIO_UNUSED,
            .bclk = bclk,
            .ws   = ws,
            .dout = dout,
            .din  = I2S_GPIO_UNUSED,
            .invert_flags = { false, false, false },
        },
    };
    if (use_apll) std.clk_cfg.clk_src = I2S_CLK_SRC_APLL;

    esp_err_t err = i2s_channel_init_std_mode(*out, &std);
    if (err != ESP_OK) {
        i2s_del_channel(*out);
        *out = NULL;
    }
    return err;
}

/* ------------------------------------------------------------- render */

static void render_task(void *arg)
{
    /* A byte-mode ring buffer can hand back any number of bytes, including a
     * count that splits a stereo frame. Accumulate first and only ever hand
     * whole frames to the DSP, otherwise the L/R interleave slips by one
     * sample and the channels swap. */
    static uint8_t acc[BLOCK_BYTES + 8];
    size_t acc_len = 0;
    size_t written;

    for (;;) {
        if (chime_active()) {
            /* A tone replaces the stream for its duration. Anything the
             * phone sends meanwhile is dropped rather than queued, so the
             * stream resumes live instead of a few hundred ms behind. */
            acc_len = 0;
            chime_render(s_in, BLOCK_FRAMES);
            dsp_process(s_in, s_w, s_t, BLOCK_FRAMES);
            i2s_channel_write(s_wfr, s_w, BLOCK_BYTES, &written, portMAX_DELAY);
            i2s_channel_write(s_twt, s_t, BLOCK_BYTES, &written, portMAX_DELAY);
            continue;
        }

        while (acc_len < BLOCK_BYTES) {
            size_t got = 0;
            uint8_t *item = xRingbufferReceiveUpTo(s_rb, &got,
                                                   pdMS_TO_TICKS(20),
                                                   BLOCK_BYTES - acc_len);
            if (!item) break;                 /* idle, or the stream stalled */
            memcpy(acc + acc_len, item, got);
            acc_len += got;
            vRingbufferReturnItem(s_rb, item);
        }

        size_t frames = acc_len / 4;          /* 2 ch * int16 */
        if (frames) {
            size_t used = frames * 4;
            memcpy(s_in, acc, used);
            acc_len -= used;
            memmove(acc, acc + used, acc_len);   /* carry the partial frame */
        } else {
            /* Underrun or idle: keep BCK/LRCK running with silence so the
             * PCM5102A PLLs stay locked and resume is click-free. */
            frames = BLOCK_FRAMES;
            memset(s_in, 0, frames * 4);
        }

        dsp_process(s_in, s_w, s_t, frames);

        i2s_channel_write(s_wfr, s_w, frames * 4, &written, portMAX_DELAY);
        i2s_channel_write(s_twt, s_t, frames * 4, &written, portMAX_DELAY);
    }
}

esp_err_t audio_out_init(uint32_t sample_rate)
{
    s_rate = sample_rate ? sample_rate : 44100;

    /* APLL keeps the two I2S units frequency-locked. Without it they run off
     * independently-divided PLL_160M and slowly drift apart, which smears the
     * woofer/tweeter phase relationship right through the crossover. */
    bool apll = true;
    esp_err_t err = make_channel(I2S_NUM_0, &s_wfr, PIN_WFR_BCK, PIN_WFR_LRCK,
                                 PIN_WFR_DIN, s_rate, apll);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "APLL unavailable (%s), falling back to default clock",
                 esp_err_to_name(err));
        apll = false;
        err = make_channel(I2S_NUM_0, &s_wfr, PIN_WFR_BCK, PIN_WFR_LRCK,
                           PIN_WFR_DIN, s_rate, apll);
    }
    if (err != ESP_OK) return err;

    err = make_channel(I2S_NUM_1, &s_twt, PIN_TWT_BCK, PIN_TWT_LRCK,
                       PIN_TWT_DIN, s_rate, apll);
    if (err != ESP_OK) return err;

    /* Preload both DMA chains with silence and enable back to back, so the
     * two DACs start within well under one sample of each other. */
    static const uint8_t silence[BLOCK_BYTES] = {0};
    size_t loaded;
    i2s_channel_preload_data(s_wfr, silence, sizeof(silence), &loaded);
    i2s_channel_preload_data(s_twt, silence, sizeof(silence), &loaded);

    ESP_RETURN_ON_ERROR(i2s_channel_enable(s_wfr), TAG, "enable wfr");
    ESP_RETURN_ON_ERROR(i2s_channel_enable(s_twt), TAG, "enable twt");

    dsp_init(s_rate);

    s_rb = xRingbufferCreate(RING_BYTES, RINGBUF_TYPE_BYTEBUF);
    if (!s_rb) return ESP_ERR_NO_MEM;

    /* Priority 5 and pinned to APP_CPU keeps it clear of the BT stack. */
    if (xTaskCreatePinnedToCore(render_task, "audio_render", 4096, NULL, 5,
                                &s_task, 1) != pdPASS)
        return ESP_ERR_NO_MEM;

    s_seq_q = xQueueCreate(4, sizeof(seq_cmd_t));
    if (!s_seq_q) return ESP_ERR_NO_MEM;
    if (xTaskCreate(seq_task, "audio_seq", 3072, NULL, 4, NULL) != pdPASS)
        return ESP_ERR_NO_MEM;

    ESP_LOGI(TAG, "I2S up: %lu Hz, 16-bit stereo, clock=%s",
             (unsigned long)s_rate, apll ? "APLL" : "PLL_160M");

    /* Clocks are running with silence — let both PLLs lock before un-muting. */
    vTaskDelay(pdMS_TO_TICKS(PLL_LOCK_MS));
    return ESP_OK;
}

static esp_err_t do_set_rate(uint32_t sample_rate)
{
    if (!sample_rate || sample_rate == s_rate) return ESP_OK;

    bool was_unmuted = s_unmuted;
    do_mute();

    i2s_channel_disable(s_wfr);
    i2s_channel_disable(s_twt);

    i2s_std_clk_config_t clk = I2S_STD_CLK_DEFAULT_CONFIG(sample_rate);
    clk.clk_src = I2S_CLK_SRC_APLL;
    if (i2s_channel_reconfig_std_clock(s_wfr, &clk) != ESP_OK ||
        i2s_channel_reconfig_std_clock(s_twt, &clk) != ESP_OK) {
        clk.clk_src = I2S_CLK_SRC_DEFAULT;
        ESP_RETURN_ON_ERROR(i2s_channel_reconfig_std_clock(s_wfr, &clk), TAG, "clk wfr");
        ESP_RETURN_ON_ERROR(i2s_channel_reconfig_std_clock(s_twt, &clk), TAG, "clk twt");
    }

    s_rate = sample_rate;
    dsp_init(s_rate);            /* rebuild filters for the new fs */

    ESP_RETURN_ON_ERROR(i2s_channel_enable(s_wfr), TAG, "re-enable wfr");
    ESP_RETURN_ON_ERROR(i2s_channel_enable(s_twt), TAG, "re-enable twt");

    ESP_LOGI(TAG, "sample rate -> %lu Hz", (unsigned long)sample_rate);

    vTaskDelay(pdMS_TO_TICKS(PLL_LOCK_MS));
    if (was_unmuted) do_unmute();
    return ESP_OK;
}


/* ---------------------------------------------------------- sequencer */

/*
 * Mute transitions take tens of milliseconds of deliberate settling, and the
 * A2DP callbacks that trigger them run on the Bluetooth stack task. Doing the
 * waiting there would stall the stack, so every transition is queued here and
 * carried out by this task instead. The render task is untouched and keeps
 * feeding the DACs throughout, so the clocks never stop.
 */
static void ring_flush(void)
{
    size_t got;
    void *item;
    while ((item = xRingbufferReceiveUpTo(s_rb, &got, 0, RING_BYTES)) != NULL)
        vRingbufferReturnItem(s_rb, item);
}

/*
 * Tones need the analog chain live, which at boot or between streams it is
 * not. Un-mute with the usual sequence, play, then put the mute state back
 * exactly as it was, so a tone never leaves the amplifiers hot.
 */
static void do_chime(chime_id_t id)
{
    bool was_unmuted = s_unmuted;
    if (!was_unmuted) do_unmute();

    chime_start(id, s_rate);

    /* Bounded wait: never hang the sequencer on a tone that misbehaves. */
    int guard_ms = 3000;
    while (chime_active() && guard_ms > 0) {
        vTaskDelay(pdMS_TO_TICKS(10));
        guard_ms -= 10;
    }
    /* Let the tail clear the DMA chain before the amps go quiet. */
    vTaskDelay(pdMS_TO_TICKS(40));

    ring_flush();
    if (!was_unmuted) do_mute();
}

static void seq_task(void *arg)
{
    seq_cmd_t c;
    for (;;) {
        if (xQueueReceive(s_seq_q, &c, portMAX_DELAY) != pdTRUE) continue;
        switch (c.op) {
        case SEQ_MUTE:   do_mute();          break;
        case SEQ_UNMUTE: do_unmute();        break;
        case SEQ_RATE:   do_set_rate(c.arg); break;
        case SEQ_CHIME:  do_chime((chime_id_t)c.arg); break;
        }
    }
}

static void seq_post(seq_op_t op, uint32_t arg)
{
    if (!s_seq_q) return;
    seq_cmd_t c = { .op = op, .arg = arg };
    xQueueSend(s_seq_q, &c, 0);
}

void audio_out_unmute(void) { seq_post(SEQ_UNMUTE, 0); }
void audio_out_mute(void)   { seq_post(SEQ_MUTE,   0); }

esp_err_t audio_out_set_rate(uint32_t sample_rate)
{
    seq_post(SEQ_RATE, sample_rate);
    return ESP_OK;
}

void audio_out_play_chime(chime_id_t id) { seq_post(SEQ_CHIME, (uint32_t)id); }

void audio_out_submit(const uint8_t *pcm, size_t bytes)
{
    if (!s_rb || !bytes) return;
    /* Never block the Bluetooth stack task: drop on overflow instead. */
    if (xRingbufferSend(s_rb, pcm, bytes, 0) != pdTRUE)
        ESP_LOGW(TAG, "ring buffer full, dropped %u bytes", (unsigned)bytes);
}
