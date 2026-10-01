#include "ble_ctl.h"
#include "dsp.h"
#include "settings.h"
#include "audio_out.h"

#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_gap_ble_api.h"
#include "esp_gatts_api.h"
#include "esp_gatt_common_api.h"

static const char *TAG = "ble_ctl";

/* 128-bit UUIDs are stored little-endian, i.e. the written form reversed. */
#define UUID128_LE(b3, b2, b1, b0) {                        \
    0x01, 0x00, 0x00, 0x00, 0x01, 0x5F, 0x4F, 0x8E,         \
    0x3D, 0x4C, 0xB2, 0xA1, (b0), (b1), (b2), (b3) }

/* 5F010000-A1B2-4C3D-8E4F-5F0100000001 */
static const uint8_t SVC_UUID[16] = UUID128_LE(0x5F, 0x01, 0x00, 0x00);
/* 5F010001-... */
static const uint8_t CFG_UUID[16] = UUID128_LE(0x5F, 0x01, 0x00, 0x01);
/* 5F010002-... */
static const uint8_t CMD_UUID[16] = UUID128_LE(0x5F, 0x01, 0x00, 0x02);

enum {
    IDX_SVC,
    IDX_CFG_DECL, IDX_CFG_VAL, IDX_CFG_CCC,
    IDX_CMD_DECL, IDX_CMD_VAL,
    IDX_NB,
};

static uint16_t s_handles[IDX_NB];
static esp_gatt_if_t s_gatts_if = ESP_GATT_IF_NONE;
static uint16_t s_conn_id;
static bool     s_connected;
static bool     s_notify_on;
static uint16_t s_mtu = 23;

/* ------------------------------------------------------------ wire format */

static inline void put_u16(uint8_t *p, uint16_t v) { p[0] = v & 0xFF; p[1] = v >> 8; }
static inline void put_i16(uint8_t *p, int16_t  v) { put_u16(p, (uint16_t)v); }
static inline uint16_t get_u16(const uint8_t *p)   { return (uint16_t)(p[0] | (p[1] << 8)); }
static inline int16_t  get_i16(const uint8_t *p)   { return (int16_t)get_u16(p); }

static void build_config(uint8_t *out)
{
    dsp_config_t c;
    dsp_get_config(&c);

    memset(out, 0, SF01_BLE_CONFIG_LEN);
    out[0] = SF01_BLE_PROTO_VERSION;
    out[1] = (uint8_t)((c.tweeter_invert ? 0x01 : 0) |
                       (audio_out_is_unmuted() ? 0x02 : 0));
    put_u16(out + 2, (uint16_t)c.crossover_hz);
    out[4] = c.volume;
    out[5] = (uint8_t)c.tweeter_delay;
    put_i16(out + 6,  (int16_t)(c.woofer_gain_db  * 10.0f));
    put_i16(out + 8,  (int16_t)(c.tweeter_gain_db * 10.0f));
    put_i16(out + 10, (int16_t)(c.master_gain_db  * 10.0f));
    put_u16(out + 12, (uint16_t)(dsp_get_sample_rate() / 100));

    for (int i = 0; i < DSP_MAX_EQ_BANDS; i++) {
        uint8_t *b = out + 14 + i * 8;
        b[0] = (uint8_t)c.eq[i].type;
        put_u16(b + 2, (uint16_t)c.eq[i].freq_hz);
        put_u16(b + 4, (uint16_t)(c.eq[i].q * 100.0f));
        put_i16(b + 6, (int16_t)(c.eq[i].gain_db * 10.0f));
    }
}

/* ---------------------------------------------------------------- commands */

enum {
    CMD_SET_XO = 0x01, CMD_SET_VOLUME, CMD_SET_GAIN, CMD_SET_EQ,
    CMD_SET_INVERT, CMD_SET_DELAY, CMD_SAVE, CMD_LOAD, CMD_DEFAULTS,
    CMD_SET_MUTE,
};

static void handle_command(const uint8_t *d, uint16_t len)
{
    if (!len) return;

    dsp_config_t c;
    dsp_get_config(&c);
    bool touched = true;

    switch (d[0]) {
    case CMD_SET_XO:
        if (len < 3) return;
        c.crossover_hz = (float)get_u16(d + 1);
        break;

    case CMD_SET_VOLUME:
        if (len < 2) return;
        c.volume = d[1] > 127 ? 127 : d[1];
        break;

    case CMD_SET_GAIN: {
        if (len < 4) return;
        float db = get_i16(d + 2) / 10.0f;
        if      (d[1] == 0) c.woofer_gain_db  = db;
        else if (d[1] == 1) c.tweeter_gain_db = db;
        else if (d[1] == 2) c.master_gain_db  = db;
        else return;
        break;
    }

    case CMD_SET_EQ: {
        if (len < 9) return;
        uint8_t band = d[1];
        if (band >= DSP_MAX_EQ_BANDS) return;
        uint8_t type = d[2];
        if (type > EQ_HIGHSHELF) return;
        c.eq[band].type    = (eq_type_t)type;
        c.eq[band].freq_hz = (float)get_u16(d + 3);
        c.eq[band].q       = get_u16(d + 5) / 100.0f;
        c.eq[band].gain_db = get_i16(d + 7) / 10.0f;
        break;
    }

    case CMD_SET_INVERT:
        if (len < 2) return;
        c.tweeter_invert = d[1] != 0;
        break;

    case CMD_SET_DELAY:
        if (len < 2) return;
        c.tweeter_delay = d[1];
        break;

    case CMD_SAVE:
        settings_save(&c);
        touched = false;
        break;

    case CMD_LOAD:
        settings_load(&c);
        break;

    case CMD_DEFAULTS:
        dsp_default_config(&c);
        settings_erase();
        break;

    case CMD_SET_MUTE:
        if (len < 2) return;
        if (d[1]) audio_out_mute(); else audio_out_unmute();
        touched = false;
        break;

    default:
        ESP_LOGW(TAG, "unknown opcode 0x%02x", d[0]);
        return;
    }

    if (touched) dsp_set_config(&c);
}

/* ------------------------------------------------------------- publishing */

/*
 * Poll rather than hooking every writer. The UART shell, AVRCP volume and the
 * mute sequencer all change state independently; watching the rendered blob
 * catches every one of them without coupling those modules to BLE.
 */
static void publish_task(void *arg)
{
    static uint8_t last[SF01_BLE_CONFIG_LEN];
    uint8_t now[SF01_BLE_CONFIG_LEN];

    build_config(last);
    esp_ble_gatts_set_attr_value(s_handles[IDX_CFG_VAL], sizeof(last), last);

    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(200));
        build_config(now);
        if (memcmp(now, last, sizeof(now)) == 0) continue;

        memcpy(last, now, sizeof(now));
        esp_ble_gatts_set_attr_value(s_handles[IDX_CFG_VAL], sizeof(now), now);

        /* A notification must fit in MTU-3; below that the phone still reads. */
        if (s_connected && s_notify_on && s_mtu >= SF01_BLE_CONFIG_LEN + 3)
            esp_ble_gatts_send_indicate(s_gatts_if, s_conn_id,
                                        s_handles[IDX_CFG_VAL],
                                        sizeof(now), now, false);
    }
}

/* ------------------------------------------------------------ attr table */

static const uint16_t PRIMARY_SERVICE_UUID = ESP_GATT_UUID_PRI_SERVICE;
static const uint16_t CHAR_DECL_UUID       = ESP_GATT_UUID_CHAR_DECLARE;
static const uint16_t CHAR_CCC_UUID        = ESP_GATT_UUID_CHAR_CLIENT_CONFIG;

static const uint8_t  PROP_READ_NOTIFY = ESP_GATT_CHAR_PROP_BIT_READ |
                                         ESP_GATT_CHAR_PROP_BIT_NOTIFY;
static const uint8_t  PROP_WRITE       = ESP_GATT_CHAR_PROP_BIT_WRITE |
                                         ESP_GATT_CHAR_PROP_BIT_WRITE_NR;

static uint8_t s_cfg_value[SF01_BLE_CONFIG_LEN];
static uint8_t s_cmd_value[16];
static uint8_t s_ccc_value[2] = {0, 0};

static const esp_gatts_attr_db_t ATTR_DB[IDX_NB] = {
    [IDX_SVC] = {{ESP_GATT_AUTO_RSP},
        {ESP_UUID_LEN_16, (uint8_t *)&PRIMARY_SERVICE_UUID, ESP_GATT_PERM_READ,
         sizeof(SVC_UUID), sizeof(SVC_UUID), (uint8_t *)SVC_UUID}},

    [IDX_CFG_DECL] = {{ESP_GATT_AUTO_RSP},
        {ESP_UUID_LEN_16, (uint8_t *)&CHAR_DECL_UUID, ESP_GATT_PERM_READ,
         sizeof(uint8_t), sizeof(uint8_t), (uint8_t *)&PROP_READ_NOTIFY}},
    [IDX_CFG_VAL] = {{ESP_GATT_AUTO_RSP},
        {ESP_UUID_LEN_128, (uint8_t *)CFG_UUID, ESP_GATT_PERM_READ,
         sizeof(s_cfg_value), sizeof(s_cfg_value), s_cfg_value}},
    [IDX_CFG_CCC] = {{ESP_GATT_AUTO_RSP},
        {ESP_UUID_LEN_16, (uint8_t *)&CHAR_CCC_UUID,
         ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
         sizeof(s_ccc_value), sizeof(s_ccc_value), s_ccc_value}},

    [IDX_CMD_DECL] = {{ESP_GATT_AUTO_RSP},
        {ESP_UUID_LEN_16, (uint8_t *)&CHAR_DECL_UUID, ESP_GATT_PERM_READ,
         sizeof(uint8_t), sizeof(uint8_t), (uint8_t *)&PROP_WRITE}},
    [IDX_CMD_VAL] = {{ESP_GATT_AUTO_RSP},
        {ESP_UUID_LEN_128, (uint8_t *)CMD_UUID, ESP_GATT_PERM_WRITE,
         sizeof(s_cmd_value), 0, s_cmd_value}},
};

/* ------------------------------------------------------------------- gap */

/*
 * A 128-bit service UUID is 18 bytes of advertising payload and the name is
 * another 15; together they overflow the 31-byte limit. Name goes in the
 * advertisement, UUID in the scan response.
 */
static esp_ble_adv_data_t s_adv_data = {
    .set_scan_rsp        = false,
    .include_name        = true,
    .include_txpower     = false,
    .min_interval        = 0x0006,
    .max_interval        = 0x0010,
    .flag                = ESP_BLE_ADV_FLAG_GEN_DISC | ESP_BLE_ADV_FLAG_BREDR_NOT_SPT,
};

static esp_ble_adv_data_t s_scan_rsp = {
    .set_scan_rsp        = true,
    .include_name        = false,
    .service_uuid_len    = sizeof(SVC_UUID),
    .p_service_uuid      = (uint8_t *)SVC_UUID,
};

static esp_ble_adv_params_t s_adv_params = {
    .adv_int_min       = 0x20,
    .adv_int_max       = 0x40,
    .adv_type          = ADV_TYPE_IND,
    .own_addr_type     = BLE_ADDR_TYPE_PUBLIC,
    .channel_map       = ADV_CHNL_ALL,
    .adv_filter_policy = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY,
};

static bool s_adv_cfg_done, s_scan_rsp_done;

static void start_adv_when_ready(void)
{
    if (s_adv_cfg_done && s_scan_rsp_done)
        esp_ble_gap_start_advertising(&s_adv_params);
}

static void gap_cb(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param)
{
    switch (event) {
    case ESP_GAP_BLE_ADV_DATA_SET_COMPLETE_EVT:
        s_adv_cfg_done = true; start_adv_when_ready(); break;
    case ESP_GAP_BLE_SCAN_RSP_DATA_SET_COMPLETE_EVT:
        s_scan_rsp_done = true; start_adv_when_ready(); break;
    case ESP_GAP_BLE_ADV_START_COMPLETE_EVT:
        if (param->adv_start_cmpl.status != ESP_BT_STATUS_SUCCESS)
            ESP_LOGE(TAG, "advertising failed to start");
        else
            ESP_LOGI(TAG, "BLE advertising");
        break;
    default:
        break;
    }
}

/* ----------------------------------------------------------------- gatts */

static void gatts_cb(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if,
                     esp_ble_gatts_cb_param_t *param)
{
    switch (event) {
    case ESP_GATTS_REG_EVT:
        s_gatts_if = gatts_if;
        esp_ble_gatts_create_attr_tab(ATTR_DB, gatts_if, IDX_NB, 0);
        break;

    case ESP_GATTS_CREAT_ATTR_TAB_EVT:
        if (param->add_attr_tab.status != ESP_GATT_OK ||
            param->add_attr_tab.num_handle != IDX_NB) {
            ESP_LOGE(TAG, "attr table failed (status %d, %d handles)",
                     param->add_attr_tab.status, param->add_attr_tab.num_handle);
            break;
        }
        memcpy(s_handles, param->add_attr_tab.handles, sizeof(s_handles));
        esp_ble_gatts_start_service(s_handles[IDX_SVC]);

        esp_ble_gap_config_adv_data(&s_adv_data);
        esp_ble_gap_config_adv_data(&s_scan_rsp);

        xTaskCreate(publish_task, "ble_publish", 3072, NULL, 3, NULL);
        break;

    case ESP_GATTS_CONNECT_EVT:
        s_conn_id   = param->connect.conn_id;
        s_connected = true;
        ESP_LOGI(TAG, "phone connected");
        break;

    case ESP_GATTS_DISCONNECT_EVT:
        s_connected = false;
        s_notify_on = false;
        s_mtu       = 23;
        ESP_LOGI(TAG, "phone disconnected, advertising again");
        esp_ble_gap_start_advertising(&s_adv_params);
        break;

    case ESP_GATTS_MTU_EVT:
        s_mtu = param->mtu.mtu;
        ESP_LOGI(TAG, "MTU %u", s_mtu);
        break;

    case ESP_GATTS_WRITE_EVT:
        if (param->write.handle == s_handles[IDX_CMD_VAL]) {
            handle_command(param->write.value, param->write.len);
        } else if (param->write.handle == s_handles[IDX_CFG_CCC] &&
                   param->write.len == 2) {
            s_notify_on = (param->write.value[0] & 0x01) != 0;
            ESP_LOGI(TAG, "notifications %s", s_notify_on ? "on" : "off");
        }
        break;

    default:
        break;
    }
}

esp_err_t ble_ctl_start(void)
{
    /* BR/EDR and BLE keep separate names; the advertisement carries this one. */
    ESP_ERROR_CHECK(esp_ble_gap_set_device_name(CONFIG_SF01_BT_DEVICE_NAME));
    ESP_ERROR_CHECK(esp_ble_gap_register_callback(gap_cb));
    ESP_ERROR_CHECK(esp_ble_gatts_register_callback(gatts_cb));
    ESP_ERROR_CHECK(esp_ble_gatts_app_register(0));
    esp_ble_gatt_set_local_mtu(247);
    ESP_LOGI(TAG, "GATT control server starting");
    return ESP_OK;
}
