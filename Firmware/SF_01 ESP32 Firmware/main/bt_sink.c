#include "bt_sink.h"
#include "audio_out.h"
#include "dsp.h"

#include <string.h>

#include "esp_log.h"
#include "esp_bt.h"
#include "esp_bt_main.h"
#include "esp_bt_device.h"
#include "esp_gap_bt_api.h"
#include "esp_a2dp_api.h"
#include "esp_avrc_api.h"

static const char *TAG = "bt_sink";

static volatile bool s_connected;

/* ------------------------------------------------------------- a2dp */

/* Runs on the Bluetooth stack task — hand the PCM off and return. */
static void a2d_data_cb(const uint8_t *data, uint32_t len)
{
    audio_out_submit(data, len);
}

static void a2d_cb(esp_a2d_cb_event_t event, esp_a2d_cb_param_t *param)
{
    switch (event) {
    case ESP_A2D_CONNECTION_STATE_EVT:
        if (param->conn_stat.state == ESP_A2D_CONNECTION_STATE_CONNECTED) {
            s_connected = true;
            ESP_LOGI(TAG, "connected");
            audio_out_play_chime(CHIME_CONNECT);
            /* Stop advertising so a second phone cannot grab the link. */
            esp_bt_gap_set_scan_mode(ESP_BT_NON_CONNECTABLE,
                                     ESP_BT_NON_DISCOVERABLE);
        } else if (param->conn_stat.state == ESP_A2D_CONNECTION_STATE_DISCONNECTED) {
            s_connected = false;
            ESP_LOGI(TAG, "disconnected");
            /* Both queue on the sequencer, so the tone finishes before the
             * amplifiers go quiet. */
            audio_out_play_chime(CHIME_DISCONNECT);
            audio_out_mute();
            esp_bt_gap_set_scan_mode(ESP_BT_CONNECTABLE,
                                     ESP_BT_GENERAL_DISCOVERABLE);
        }
        break;

    case ESP_A2D_AUDIO_STATE_EVT:
        /* Only STARTED is matched by name: the "stopped" and "suspended"
         * enumerators have been renamed across IDF releases, and anything
         * that is not STARTED should mute anyway. */
        if (param->audio_stat.state == ESP_A2D_AUDIO_STATE_STARTED) {
            ESP_LOGI(TAG, "stream started");
            audio_out_unmute();
        } else {
            ESP_LOGI(TAG, "stream stopped/suspended");
            audio_out_mute();
        }
        break;

    case ESP_A2D_AUDIO_CFG_EVT:
        if (param->audio_cfg.mcc.type == ESP_A2D_MCT_SBC) {
            /* samp_freq holds bits 7..4 of SBC capability octet 0, shifted
             * down: bit3=16k, bit2=32k, bit1=44.1k, bit0=48k. */
            uint8_t sf  = param->audio_cfg.mcc.cie.sbc_info.samp_freq;
            uint32_t sr = 16000;
            if      (sf & (1 << 2)) sr = 32000;
            else if (sf & (1 << 1)) sr = 44100;
            else if (sf & (1 << 0)) sr = 48000;
            ESP_LOGI(TAG, "A2DP sample rate %lu Hz", (unsigned long)sr);
            audio_out_set_rate(sr);
        }
        break;

    default:
        break;
    }
}

/* ------------------------------------------------------------- avrcp */

static void set_volume(uint8_t vol)
{
    dsp_config_t c;
    dsp_get_config(&c);
    c.volume = vol > 127 ? 127 : vol;
    dsp_set_config(&c);
    ESP_LOGI(TAG, "volume -> %u/127", c.volume);
}

static void avrc_tg_cb(esp_avrc_tg_cb_event_t event, esp_avrc_tg_cb_param_t *param)
{
    switch (event) {
    case ESP_AVRC_TG_SET_ABSOLUTE_VOLUME_CMD_EVT:
        set_volume(param->set_abs_vol.volume);
        break;

    case ESP_AVRC_TG_REGISTER_NOTIFICATION_EVT:
        if (param->reg_ntf.event_id == ESP_AVRC_RN_VOLUME_CHANGE) {
            dsp_config_t c;
            dsp_get_config(&c);
            esp_avrc_rn_param_t rn = { .volume = c.volume };
            esp_avrc_tg_send_rn_rsp(ESP_AVRC_RN_VOLUME_CHANGE,
                                    ESP_AVRC_RN_RSP_INTERIM, &rn);
        }
        break;

    default:
        break;
    }
}

static void avrc_ct_cb(esp_avrc_ct_cb_event_t event, esp_avrc_ct_cb_param_t *param)
{
    (void)event; (void)param;   /* metadata/passthrough not used yet */
}

/* --------------------------------------------------------------- gap */

static void gap_cb(esp_bt_gap_cb_event_t event, esp_bt_gap_cb_param_t *param)
{
    switch (event) {
    case ESP_BT_GAP_AUTH_CMPL_EVT:
        if (param->auth_cmpl.stat == ESP_BT_STATUS_SUCCESS)
            ESP_LOGI(TAG, "paired with %s", param->auth_cmpl.device_name);
        else
            ESP_LOGW(TAG, "pairing failed, status %d", param->auth_cmpl.stat);
        break;

    case ESP_BT_GAP_CFM_REQ_EVT:
        /* "Just works" — no display or keypad on this board. */
        esp_bt_gap_ssp_confirm_reply(param->cfm_req.bda, true);
        break;

    default:
        break;
    }
}

/* -------------------------------------------------------------- init */

esp_err_t bt_sink_start(const char *device_name)
{
    /* Dual mode: BR/EDR carries A2DP audio, BLE carries the control
     * service. BLE controller memory is NOT released here — doing so would
     * make the GATT server impossible for the rest of this boot. */
    esp_bt_controller_config_t cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_bt_controller_init(&cfg));
    ESP_ERROR_CHECK(esp_bt_controller_enable(ESP_BT_MODE_BTDM));

    ESP_ERROR_CHECK(esp_bluedroid_init());
    ESP_ERROR_CHECK(esp_bluedroid_enable());

    ESP_ERROR_CHECK(esp_bt_gap_register_callback(gap_cb));

    ESP_ERROR_CHECK(esp_avrc_ct_init());
    ESP_ERROR_CHECK(esp_avrc_ct_register_callback(avrc_ct_cb));
    ESP_ERROR_CHECK(esp_avrc_tg_init());
    ESP_ERROR_CHECK(esp_avrc_tg_register_callback(avrc_tg_cb));

    /* Advertise absolute-volume support so phones drive our digital gain. */
    esp_avrc_rn_evt_cap_mask_t caps = {0};
    esp_avrc_rn_evt_bit_mask_operation(ESP_AVRC_BIT_MASK_OP_SET, &caps,
                                       ESP_AVRC_RN_VOLUME_CHANGE);
    ESP_ERROR_CHECK(esp_avrc_tg_set_rn_evt_cap(&caps));

    ESP_ERROR_CHECK(esp_a2d_register_callback(a2d_cb));
    ESP_ERROR_CHECK(esp_a2d_sink_register_data_callback(a2d_data_cb));
    ESP_ERROR_CHECK(esp_a2d_sink_init());

    /* No display and no keypad, so pairing is "just works". */
    esp_bt_io_cap_t iocap = ESP_BT_IO_CAP_NONE;
    esp_bt_gap_set_security_param(ESP_BT_SP_IOCAP_MODE, &iocap, sizeof(iocap));

    ESP_ERROR_CHECK(esp_bt_gap_set_device_name(device_name));
    ESP_ERROR_CHECK(esp_bt_gap_set_scan_mode(ESP_BT_CONNECTABLE,
                                             ESP_BT_GENERAL_DISCOVERABLE));

    ESP_LOGI(TAG, "A2DP sink \"%s\" discoverable", device_name);
    return ESP_OK;
}

bool bt_sink_is_connected(void) { return s_connected; }
