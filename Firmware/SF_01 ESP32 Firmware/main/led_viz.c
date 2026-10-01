#include "led_viz.h"
#include "dsp.h"
#include "audio_out.h"

#include <math.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/ledc.h"
#include "esp_log.h"

static const char *TAG = "led_viz";

/* A Kconfig bool set to 'n' is simply undefined rather than 0. */
#ifndef CONFIG_SF01_LED_ACTIVE_LOW
#define CONFIG_SF01_LED_ACTIVE_LOW 0
#endif

#define LEDC_MODE       LEDC_LOW_SPEED_MODE
#define LEDC_TIMER      LEDC_TIMER_0
#define LEDC_CHANNEL    LEDC_CHANNEL_0
#define LEDC_RES        LEDC_TIMER_10_BIT
#define LEDC_MAX        1023
#define LEDC_FREQ_HZ    5000

#define TICK_HZ         60
#define TICK_MS         (1000 / TICK_HZ)

static gpio_num_t s_pin = (gpio_num_t)CONFIG_SF01_LED_GPIO;
static bool       s_active_low = false;
static float      s_override = -1.0f;

static void set_brightness(float level)
{
    if (level < 0.0f) level = 0.0f;
    if (level > 1.0f) level = 1.0f;

    /* LED brightness is perceived roughly as duty^(1/2.2), so raise the
     * level to 2.2 to make the pulse look linear rather than blowing out. */
    float duty_f = powf(level, 2.2f) * (float)LEDC_MAX;
    uint32_t duty = (uint32_t)(duty_f + 0.5f);
    if (s_active_low) duty = LEDC_MAX - duty;

    ledc_set_duty(LEDC_MODE, LEDC_CHANNEL, duty);
    ledc_update_duty(LEDC_MODE, LEDC_CHANNEL);
}

static void viz_task(void *arg)
{
    /* The bass band of an already gain-scaled signal rarely approaches full
     * scale, and how close it gets depends on volume and EQ. Track a slowly
     * decaying peak and normalise against it, so the pulse uses the LED's
     * whole range at any listening level. The floor stops near-silence from
     * being amplified into flicker. */
    float peak = 0.05f;
    float phase = 0.0f;

    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(TICK_MS));

        if (s_override >= 0.0f) { set_brightness(s_override); continue; }

        if (!audio_out_is_unmuted()) {
            /* Idle: a slow, dim breath so the speaker still looks alive. */
            phase += 2.0f * (float)M_PI / (TICK_HZ * 4.0f);   /* 4 s period */
            if (phase > 2.0f * (float)M_PI) phase -= 2.0f * (float)M_PI;
            set_brightness(0.06f * (0.5f + 0.5f * sinf(phase)));
            peak = 0.05f;
            continue;
        }

        float level = dsp_bass_level();
        if (level > peak) peak = level;
        else              peak *= 0.998f;            /* ~5 s decay */
        if (peak < 0.02f) peak = 0.02f;

        set_brightness(level / peak);
    }
}

esp_err_t led_viz_set_pin(gpio_num_t gpio, bool active_low)
{
    if (!GPIO_IS_VALID_OUTPUT_GPIO(gpio)) return ESP_ERR_INVALID_ARG;

    s_pin = gpio;
    s_active_low = active_low;

    ledc_channel_config_t ch = {
        .gpio_num   = gpio,
        .speed_mode = LEDC_MODE,
        .channel    = LEDC_CHANNEL,
        .timer_sel  = LEDC_TIMER,
        .duty       = active_low ? LEDC_MAX : 0,
        .hpoint     = 0,
        .intr_type  = LEDC_INTR_DISABLE,
    };
    esp_err_t err = ledc_channel_config(&ch);
    if (err == ESP_OK)
        ESP_LOGI(TAG, "LED on GPIO%d (%s)", gpio, active_low ? "active low" : "active high");
    return err;
}

esp_err_t led_viz_start(void)
{
    ledc_timer_config_t t = {
        .speed_mode      = LEDC_MODE,
        .duty_resolution = LEDC_RES,
        .timer_num       = LEDC_TIMER,
        .freq_hz         = LEDC_FREQ_HZ,
        .clk_cfg         = LEDC_AUTO_CLK,
    };
    esp_err_t err = ledc_timer_config(&t);
    if (err != ESP_OK) return err;

    err = led_viz_set_pin((gpio_num_t)CONFIG_SF01_LED_GPIO,
                          CONFIG_SF01_LED_ACTIVE_LOW);
    if (err != ESP_OK) return err;

    if (xTaskCreate(viz_task, "led_viz", 2560, NULL, 2, NULL) != pdPASS)
        return ESP_ERR_NO_MEM;

    return ESP_OK;
}

void led_viz_override(float level) { s_override = level; }
gpio_num_t led_viz_get_pin(void)   { return s_pin; }
bool led_viz_is_active_low(void)   { return s_active_low; }
