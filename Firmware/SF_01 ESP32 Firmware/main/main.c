/*
 * SF_01 Speaker Frame — ESP32 audio firmware.
 *
 *   Bluetooth A2DP sink  ->  EQ  ->  LR4 crossover  ->  2x I2S  ->  2x PCM5102A
 *
 * Target: ESP32-WROVER-E-N16R8 (U28 on the SF_01 Compute Board). Bluetooth
 * Classic is required for A2DP, so this will not run on an ESP32-S3.
 */

#include "board.h"
#include "audio_out.h"
#include "bt_sink.h"
#include "ble_ctl.h"
#include "led_viz.h"
#include "dsp.h"
#include "settings.h"
#include "shell.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"
#include "esp_log.h"

static const char *TAG = "sf01";

void app_main(void)
{
    /*
     * This must come first, before anything that can block.
     *
     * DRIVER_MUTE is held low by the five 100 k pull-downs on the board
     * (R20/R27/R36/R46/R56), and the amplifiers' SDZ pins are strapped to
     * FAULTZ, so from the moment PVCC rises the amps are enabled and
     * un-muted while the DACs' XSMT pin is still floating. Asserting mute
     * here closes that window as early as firmware possibly can.
     */
    audio_out_safe_state();

    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);

    dsp_config_t cfg;
    settings_load(&cfg);

    /* Starts both I2S channels feeding silence and waits for the PCM5102A
     * PLLs to lock. Output stays muted until a stream actually starts. */
    ESP_ERROR_CHECK(audio_out_init(44100));
    dsp_set_config(&cfg);

    /* Queued, so it rings while the Bluetooth stack comes up rather than
     * delaying boot. */
    audio_out_play_chime(CHIME_STARTUP);

    ESP_ERROR_CHECK(bt_sink_start(CONFIG_SF01_BT_DEVICE_NAME));
    ESP_ERROR_CHECK(ble_ctl_start());
    ESP_ERROR_CHECK(led_viz_start());

    shell_start();

    ESP_LOGI(TAG, "ready — pair with \"%s\", type 'help' for tuning commands",
             CONFIG_SF01_BT_DEVICE_NAME);
}
