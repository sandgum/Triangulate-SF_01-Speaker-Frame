#include "settings.h"

#include <string.h>
#include "nvs.h"
#include "nvs_flash.h"
#include "esp_log.h"

static const char *TAG = "settings";

#define NS       "sf01"
#define KEY_CFG  "dsp"
/* Bump when dsp_config_t changes shape, so old blobs are discarded. */
#define CFG_VER  1

typedef struct {
    uint16_t     ver;
    uint16_t     size;
    dsp_config_t cfg;
} blob_t;

void settings_load(dsp_config_t *cfg)
{
    dsp_default_config(cfg);

    nvs_handle_t h;
    if (nvs_open(NS, NVS_READONLY, &h) != ESP_OK) {
        ESP_LOGI(TAG, "no saved tuning, using defaults");
        return;
    }

    blob_t b;
    size_t len = sizeof(b);
    esp_err_t err = nvs_get_blob(h, KEY_CFG, &b, &len);
    nvs_close(h);

    if (err != ESP_OK || len != sizeof(b) ||
        b.ver != CFG_VER || b.size != sizeof(dsp_config_t)) {
        ESP_LOGW(TAG, "saved tuning missing or incompatible, using defaults");
        return;
    }

    *cfg = b.cfg;
    ESP_LOGI(TAG, "loaded saved tuning");
}

esp_err_t settings_save(const dsp_config_t *cfg)
{
    nvs_handle_t h;
    esp_err_t err = nvs_open(NS, NVS_READWRITE, &h);
    if (err != ESP_OK) return err;

    blob_t b = { .ver = CFG_VER, .size = sizeof(dsp_config_t), .cfg = *cfg };
    err = nvs_set_blob(h, KEY_CFG, &b, sizeof(b));
    if (err == ESP_OK) err = nvs_commit(h);
    nvs_close(h);

    if (err == ESP_OK) ESP_LOGI(TAG, "tuning saved");
    return err;
}

esp_err_t settings_erase(void)
{
    nvs_handle_t h;
    esp_err_t err = nvs_open(NS, NVS_READWRITE, &h);
    if (err != ESP_OK) return err;
    err = nvs_erase_key(h, KEY_CFG);
    if (err == ESP_ERR_NVS_NOT_FOUND) err = ESP_OK;
    if (err == ESP_OK) err = nvs_commit(h);
    nvs_close(h);
    return err;
}
