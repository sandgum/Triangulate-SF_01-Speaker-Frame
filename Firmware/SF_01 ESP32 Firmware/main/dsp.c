#include "dsp.h"

#include <math.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "esp_log.h"

static const char *TAG = "dsp";

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define NCH 2  /* left, right */

/* ---------------------------------------------------------------- biquad */

typedef struct { float b0, b1, b2, a1, a2; } biquad_t;   /* a0-normalised */
typedef struct { float z1, z2; }             bqstate_t;

/* Transposed direct form II — good float behaviour, two states. */
static inline float bq(const biquad_t *c, bqstate_t *s, float x)
{
    float y = c->b0 * x + s->z1;
    s->z1   = c->b1 * x - c->a1 * y + s->z2;
    s->z2   = c->b2 * x - c->a2 * y;
    return y;
}

static void bq_bypass(biquad_t *c)
{
    c->b0 = 1.0f; c->b1 = 0.0f; c->b2 = 0.0f; c->a1 = 0.0f; c->a2 = 0.0f;
}

/* RBJ audio EQ cookbook. */
static void bq_lowpass(biquad_t *c, float fs, float f0, float q)
{
    float w0 = 2.0f * (float)M_PI * f0 / fs;
    float cw = cosf(w0), sw = sinf(w0);
    float alpha = sw / (2.0f * q);
    float a0 = 1.0f + alpha;
    c->b0 = ((1.0f - cw) * 0.5f) / a0;
    c->b1 = (1.0f - cw) / a0;
    c->b2 = c->b0;
    c->a1 = (-2.0f * cw) / a0;
    c->a2 = (1.0f - alpha) / a0;
}

static void bq_highpass(biquad_t *c, float fs, float f0, float q)
{
    float w0 = 2.0f * (float)M_PI * f0 / fs;
    float cw = cosf(w0), sw = sinf(w0);
    float alpha = sw / (2.0f * q);
    float a0 = 1.0f + alpha;
    c->b0 = ((1.0f + cw) * 0.5f) / a0;
    c->b1 = (-(1.0f + cw)) / a0;
    c->b2 = c->b0;
    c->a1 = (-2.0f * cw) / a0;
    c->a2 = (1.0f - alpha) / a0;
}

static void bq_peaking(biquad_t *c, float fs, float f0, float q, float gain_db)
{
    float A  = powf(10.0f, gain_db / 40.0f);
    float w0 = 2.0f * (float)M_PI * f0 / fs;
    float cw = cosf(w0), sw = sinf(w0);
    float alpha = sw / (2.0f * q);
    float a0 = 1.0f + alpha / A;
    c->b0 = (1.0f + alpha * A) / a0;
    c->b1 = (-2.0f * cw) / a0;
    c->b2 = (1.0f - alpha * A) / a0;
    c->a1 = c->b1;
    c->a2 = (1.0f - alpha / A) / a0;
}

static void bq_lowshelf(biquad_t *c, float fs, float f0, float q, float gain_db)
{
    float A  = powf(10.0f, gain_db / 40.0f);
    float w0 = 2.0f * (float)M_PI * f0 / fs;
    float cw = cosf(w0), sw = sinf(w0);
    float alpha = sw / (2.0f * q);
    float sq = 2.0f * sqrtf(A) * alpha;
    float a0 = (A + 1.0f) + (A - 1.0f) * cw + sq;
    c->b0 = (A * ((A + 1.0f) - (A - 1.0f) * cw + sq)) / a0;
    c->b1 = (2.0f * A * ((A - 1.0f) - (A + 1.0f) * cw)) / a0;
    c->b2 = (A * ((A + 1.0f) - (A - 1.0f) * cw - sq)) / a0;
    c->a1 = (-2.0f * ((A - 1.0f) + (A + 1.0f) * cw)) / a0;
    c->a2 = ((A + 1.0f) + (A - 1.0f) * cw - sq) / a0;
}

static void bq_highshelf(biquad_t *c, float fs, float f0, float q, float gain_db)
{
    float A  = powf(10.0f, gain_db / 40.0f);
    float w0 = 2.0f * (float)M_PI * f0 / fs;
    float cw = cosf(w0), sw = sinf(w0);
    float alpha = sw / (2.0f * q);
    float sq = 2.0f * sqrtf(A) * alpha;
    float a0 = (A + 1.0f) - (A - 1.0f) * cw + sq;
    c->b0 = (A * ((A + 1.0f) + (A - 1.0f) * cw + sq)) / a0;
    c->b1 = (-2.0f * A * ((A - 1.0f) + (A + 1.0f) * cw)) / a0;
    c->b2 = (A * ((A + 1.0f) + (A - 1.0f) * cw - sq)) / a0;
    c->a1 = (2.0f * ((A - 1.0f) - (A + 1.0f) * cw)) / a0;
    c->a2 = ((A + 1.0f) - (A - 1.0f) * cw - sq) / a0;
}

/* ------------------------------------------------------------- dsp state */

/* LR4 = two cascaded Butterworth sections. */
#define LR_SECTIONS 2
#define BUTTERWORTH_Q 0.70710678f

static struct {
    uint32_t    fs;

    biquad_t    eq[DSP_MAX_EQ_BANDS];
    biquad_t    lp[LR_SECTIONS];
    biquad_t    hp[LR_SECTIONS];

    bqstate_t   eq_s[NCH][DSP_MAX_EQ_BANDS];
    bqstate_t   lp_s[NCH][LR_SECTIONS];
    bqstate_t   hp_s[NCH][LR_SECTIONS];

    float       dly[NCH][DSP_MAX_DELAY_SAMPLES];
    uint16_t    dly_idx;

    biquad_t    bass[2];           /* ~120 Hz tap for the LED visualiser */
    bqstate_t   bass_s[2];
    float       bass_env;
    float       bass_atk, bass_rel;

    float       g_wfr, g_twt;      /* linear, includes master + volume */
    float       twt_sign;
    uint16_t    twt_delay;
    uint8_t     n_eq;              /* active band count                */
} s;

static dsp_config_t     s_cfg;
static SemaphoreHandle_t s_lock;
static volatile bool     s_dirty = true;

void dsp_default_config(dsp_config_t *cfg)
{
    memset(cfg, 0, sizeof(*cfg));

    /* 3.5" mid-bass to 0.75" tweeter. 2.8 kHz keeps the tweeter well
     * above its Fs and the woofer below its cone break-up. Re-tune by
     * measurement — this is a safe starting point, not a final answer. */
    cfg->crossover_hz    = 2800.0f;
    cfg->woofer_gain_db  = 0.0f;
    cfg->tweeter_gain_db = -3.0f;   /* tweeters usually run hotter */
    cfg->master_gain_db  = -3.0f;   /* headroom for EQ boost       */
    cfg->volume          = 96;
    cfg->tweeter_invert  = false;
    cfg->tweeter_delay   = 0;

    cfg->eq[0] = (eq_band_t){ EQ_OFF, 100.0f,  0.7f, 0.0f };
    cfg->eq[1] = (eq_band_t){ EQ_OFF, 1000.0f, 1.0f, 0.0f };
    cfg->eq[2] = (eq_band_t){ EQ_OFF, 8000.0f, 0.7f, 0.0f };
}

static float db2lin(float db) { return powf(10.0f, db / 20.0f); }

static float volume_to_gain(uint8_t v)
{
    if (v == 0) return 0.0f;
    if (v > 127) v = 127;
    /* -45 dB at 1, 0 dB at 127 — roughly perceptually even. */
    return db2lin(-45.0f * (1.0f - (float)v / 127.0f));
}

static void recompute(void)
{
    float fs = (float)s.fs;
    dsp_config_t c;

    xSemaphoreTake(s_lock, portMAX_DELAY);
    c = s_cfg;
    xSemaphoreGive(s_lock);

    float fx = c.crossover_hz;
    if (fx < 200.0f)         fx = 200.0f;
    if (fx > fs * 0.45f)     fx = fs * 0.45f;

    for (int i = 0; i < LR_SECTIONS; i++) {
        bq_lowpass (&s.lp[i], fs, fx, BUTTERWORTH_Q);
        bq_highpass(&s.hp[i], fs, fx, BUTTERWORTH_Q);
    }

    s.n_eq = 0;
    for (int i = 0; i < DSP_MAX_EQ_BANDS; i++) {
        const eq_band_t *b = &c.eq[i];
        float f = b->freq_hz, q = b->q;
        if (f < 20.0f)       f = 20.0f;
        if (f > fs * 0.45f)  f = fs * 0.45f;
        if (q < 0.1f)        q = 0.1f;
        if (q > 10.0f)       q = 10.0f;

        switch (b->type) {
        case EQ_PEAKING:   bq_peaking  (&s.eq[i], fs, f, q, b->gain_db); break;
        case EQ_LOWSHELF:  bq_lowshelf (&s.eq[i], fs, f, q, b->gain_db); break;
        case EQ_HIGHSHELF: bq_highshelf(&s.eq[i], fs, f, q, b->gain_db); break;
        default:           bq_bypass(&s.eq[i]);                          break;
        }
        if (b->type != EQ_OFF) s.n_eq = i + 1;   /* process up to last active */
    }

    /* Visualiser tap: 24 dB/oct at 120 Hz, then a fast-attack slow-release
     * envelope so the LED snaps to a kick and decays smoothly rather than
     * flickering at the waveform rate. */
    for (int i = 0; i < 2; i++)
        bq_lowpass(&s.bass[i], fs, 120.0f, BUTTERWORTH_Q);
    s.bass_atk = 1.0f - expf(-1.0f / (fs * 0.004f));   /* 4 ms   */
    s.bass_rel = 1.0f - expf(-1.0f / (fs * 0.180f));   /* 180 ms */

    float vol = volume_to_gain(c.volume) * db2lin(c.master_gain_db);
    s.g_wfr    = vol * db2lin(c.woofer_gain_db);
    s.g_twt    = vol * db2lin(c.tweeter_gain_db);
    s.twt_sign = c.tweeter_invert ? -1.0f : 1.0f;
    s.twt_delay = c.tweeter_delay < DSP_MAX_DELAY_SAMPLES
                ? c.tweeter_delay : DSP_MAX_DELAY_SAMPLES - 1;

    ESP_LOGI(TAG, "fs=%lu xo=%.0fHz wfr=%+.1fdB twt=%+.1fdB vol=%u eq_bands=%u",
             (unsigned long)s.fs, fx, c.woofer_gain_db, c.tweeter_gain_db,
             c.volume, s.n_eq);
}

void dsp_init(uint32_t sample_rate)
{
    if (!s_lock) {
        s_lock = xSemaphoreCreateMutex();
        dsp_default_config(&s_cfg);
    }
    /* Keep the config; only the rate-dependent parts are rebuilt. */
    memset(s.eq_s, 0, sizeof(s.eq_s));
    memset(s.lp_s, 0, sizeof(s.lp_s));
    memset(s.hp_s, 0, sizeof(s.hp_s));
    memset(s.dly,  0, sizeof(s.dly));
    memset(s.bass_s, 0, sizeof(s.bass_s));
    s.bass_env = 0.0f;
    s.dly_idx = 0;
    s.fs = sample_rate ? sample_rate : 44100;
    s_dirty = true;
}

void dsp_set_config(const dsp_config_t *cfg)
{
    xSemaphoreTake(s_lock, portMAX_DELAY);
    s_cfg = *cfg;
    xSemaphoreGive(s_lock);
    s_dirty = true;      /* picked up at the next block boundary */
}

void dsp_get_config(dsp_config_t *cfg)
{
    xSemaphoreTake(s_lock, portMAX_DELAY);
    *cfg = s_cfg;
    xSemaphoreGive(s_lock);
}

uint32_t dsp_get_sample_rate(void) { return s.fs; }

/* Cubic soft knee: unity slope at 0, maps +/-1.5 to +/-1.0, flat beyond.
 * Stops EQ boost from turning into hard digital clipping. */
static inline float soft_clip(float x)
{
    if (x >=  1.5f) return  1.0f;
    if (x <= -1.5f) return -1.0f;
    return x - (4.0f / 27.0f) * x * x * x;
}

static inline int16_t to_i16(float x)
{
    float y = soft_clip(x) * 32767.0f;
    return (int16_t)(y >= 0.0f ? y + 0.5f : y - 0.5f);
}

void dsp_process(const int16_t *in, int16_t *woofer, int16_t *tweeter, size_t frames)
{
    if (s_dirty) { s_dirty = false; recompute(); }

    const uint8_t  neq   = s.n_eq;
    const uint16_t delay = s.twt_delay;

    for (size_t n = 0; n < frames; n++) {
        float lo_mono = 0.0f;

        for (int ch = 0; ch < NCH; ch++) {
            float x = (float)in[n * NCH + ch] * (1.0f / 32768.0f);

            for (uint8_t i = 0; i < neq; i++)
                x = bq(&s.eq[i], &s.eq_s[ch][i], x);

            float lo = x, hi = x;
            for (int i = 0; i < LR_SECTIONS; i++) {
                lo = bq(&s.lp[i], &s.lp_s[ch][i], lo);
                hi = bq(&s.hp[i], &s.hp_s[ch][i], hi);
            }

            /* Integer-sample time alignment on the tweeter branch. */
            if (delay) {
                uint16_t w = s.dly_idx;
                uint16_t r = (uint16_t)((w + DSP_MAX_DELAY_SAMPLES - delay)
                                        % DSP_MAX_DELAY_SAMPLES);
                s.dly[ch][w] = hi;
                hi = s.dly[ch][r];
            }

            float lo_out = lo * s.g_wfr;
            lo_mono += lo_out;

            woofer [n * NCH + ch] = to_i16(lo_out);
            tweeter[n * NCH + ch] = to_i16(hi * s.g_twt * s.twt_sign);
        }

        float b = lo_mono * 0.5f;
        for (int i = 0; i < 2; i++)
            b = bq(&s.bass[i], &s.bass_s[i], b);
        float mag = fabsf(b);
        s.bass_env += (mag > s.bass_env ? s.bass_atk : s.bass_rel)
                    * (mag - s.bass_env);

        if (delay)
            s.dly_idx = (uint16_t)((s.dly_idx + 1) % DSP_MAX_DELAY_SAMPLES);
    }
}

float dsp_bass_level(void)
{
    float v = s.bass_env;
    return v < 0.0f ? 0.0f : (v > 1.0f ? 1.0f : v);
}
