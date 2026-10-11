#include "ble_ctl.h"
#include "dsp.h"
#include "settings.h"
#include "audio_out.h"
#include "fw_update.h"

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
/* 5F010003/4/5-... firmware update, see fw_update.h */
static const uint8_t FWC_UUID[16] = UUID128_LE(0x5F, 0x01, 0x00, 0x03);
static const uint8_t FWD_UUID[16] = UUID128_LE(0x5F, 0x01, 0x00, 0x04);
static const uint8_t FWI_UUID[16] = UUID128_LE(0x5F, 0x01, 0x00, 0x05);

enum {
    IDX_SVC,
    IDX_CFG_DECL, IDX_CFG_VAL, IDX_CFG_CCC,
    IDX_CMD_DECL, IDX_CMD_VAL,
    /* Firmware update. Appended, never inserted: the handles above must not
     * move, or a phone holding a cached attribute table writes to the wrong
     * characteristic. */
    IDX_FWC_DECL, IDX_FWC_VAL, IDX_FWC_CCC,
    IDX_FWD_DECL, IDX_FWD_VAL,
    IDX_FWI_DECL, IDX_FWI_VAL,
    IDX_NB,
};

static uint16_t s_handles[IDX_NB];
static esp_gatt_if_t s_gatts_if = ESP_GATT_IF_NONE;

/*
 * Two connections, so the Mac can push firmware while the phone app is open.
 * Each keeps its own MTU and subscriptions.
 */
#define MAX_CONNS 2

typedef struct {
    bool          used;
    uint16_t      id;
    esp_bd_addr_t bda;
    uint16_t      mtu;
    bool          cfg_notify;
    bool          fw_notify;
} conn_t;

static conn_t s_conns[MAX_CONNS];
static bool   s_advertising;

static conn_t *conn_find(uint16_t id)
{
    for (int i = 0; i < MAX_CONNS; i++)
        if (s_conns[i].used && s_conns[i].id == id) return &s_conns[i];
    return NULL;
}

static conn_t *conn_alloc(uint16_t id)
{
    for (int i = 0; i < MAX_CONNS; i++) {
        if (!s_conns[i].used) {
            memset(&s_conns[i], 0, sizeof(s_conns[i]));
            s_conns[i].used = true;
            s_conns[i].id   = id;
            s_conns[i].mtu  = 23;
            return &s_conns[i];
        }
    }
    return NULL;
}

static int conn_count(void)
{
    int n = 0;
    for (int i = 0; i < MAX_CONNS; i++) n += s_conns[i].used ? 1 : 0;
    return n;
}

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
    static uint8_t fw_last[FW_INFO_LEN];
    uint8_t now[SF01_BLE_CONFIG_LEN];
    uint8_t fw_now[FW_INFO_LEN];

    build_config(last);
    esp_ble_gatts_set_attr_value(s_handles[IDX_CFG_VAL], sizeof(last), last);
    fw_update_info(fw_last);
    esp_ble_gatts_set_attr_value(s_handles[IDX_FWI_VAL], sizeof(fw_last), fw_last);

    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(200));

        fw_update_info(fw_now);
        if (memcmp(fw_now, fw_last, sizeof(fw_now)) != 0) {
            memcpy(fw_last, fw_now, sizeof(fw_now));
            esp_ble_gatts_set_attr_value(s_handles[IDX_FWI_VAL], sizeof(fw_now), fw_now);
        }

        build_config(now);
        if (memcmp(now, last, sizeof(now)) == 0) continue;

        memcpy(last, now, sizeof(now));
        esp_ble_gatts_set_attr_value(s_handles[IDX_CFG_VAL], sizeof(now), now);

        /* A notification must fit in MTU-3; below that the phone still reads. */
        for (int i = 0; i < MAX_CONNS; i++) {
            const conn_t *c = &s_conns[i];
            if (c->used && c->cfg_notify && c->mtu >= SF01_BLE_CONFIG_LEN + 3)
                esp_ble_gatts_send_indicate(s_gatts_if, c->id,
                                            s_handles[IDX_CFG_VAL],
                                            sizeof(now), now, false);
        }
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

static const uint8_t  PROP_WRITE_NOTIFY = ESP_GATT_CHAR_PROP_BIT_WRITE |
                                          ESP_GATT_CHAR_PROP_BIT_NOTIFY;
static const uint8_t  PROP_WRITE_NR     = ESP_GATT_CHAR_PROP_BIT_WRITE_NR;
static const uint8_t  PROP_READ         = ESP_GATT_CHAR_PROP_BIT_READ;

static uint8_t s_cfg_value[SF01_BLE_CONFIG_LEN];
static uint8_t s_cmd_value[16];
static uint8_t s_ccc_value[2] = {0, 0};
static uint8_t s_fwc_value[8];
static uint8_t s_fwc_ccc[2] = {0, 0};
static uint8_t s_fwi_value[FW_INFO_LEN];

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

    [IDX_FWC_DECL] = {{ESP_GATT_AUTO_RSP},
        {ESP_UUID_LEN_16, (uint8_t *)&CHAR_DECL_UUID, ESP_GATT_PERM_READ,
         sizeof(uint8_t), sizeof(uint8_t), (uint8_t *)&PROP_WRITE_NOTIFY}},
    [IDX_FWC_VAL] = {{ESP_GATT_AUTO_RSP},
        {ESP_UUID_LEN_128, (uint8_t *)FWC_UUID, ESP_GATT_PERM_WRITE,
         sizeof(s_fwc_value), 0, s_fwc_value}},
    [IDX_FWC_CCC] = {{ESP_GATT_AUTO_RSP},
        {ESP_UUID_LEN_16, (uint8_t *)&CHAR_CCC_UUID,
         ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
         sizeof(s_fwc_ccc), sizeof(s_fwc_ccc), s_fwc_ccc}},

    /* Image bytes go straight to the application: the stack keeps no copy,
     * and write-without-response needs no reply. */
    [IDX_FWD_DECL] = {{ESP_GATT_AUTO_RSP},
        {ESP_UUID_LEN_16, (uint8_t *)&CHAR_DECL_UUID, ESP_GATT_PERM_READ,
         sizeof(uint8_t), sizeof(uint8_t), (uint8_t *)&PROP_WRITE_NR}},
    [IDX_FWD_VAL] = {{ESP_GATT_RSP_BY_APP},
        {ESP_UUID_LEN_128, (uint8_t *)FWD_UUID, ESP_GATT_PERM_WRITE,
         512, 0, NULL}},

    [IDX_FWI_DECL] = {{ESP_GATT_AUTO_RSP},
        {ESP_UUID_LEN_16, (uint8_t *)&CHAR_DECL_UUID, ESP_GATT_PERM_READ,
         sizeof(uint8_t), sizeof(uint8_t), (uint8_t *)&PROP_READ}},
    [IDX_FWI_VAL] = {{ESP_GATT_AUTO_RSP},
        {ESP_UUID_LEN_128, (uint8_t *)FWI_UUID, ESP_GATT_PERM_READ,
         sizeof(s_fwi_value), sizeof(s_fwi_value), s_fwi_value}},
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
    if (s_adv_cfg_done && s_scan_rsp_done && !s_advertising)
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
        if (param->adv_start_cmpl.status != ESP_BT_STATUS_SUCCESS) {
            ESP_LOGE(TAG, "advertising failed to start");
        } else {
            s_advertising = true;
            ESP_LOGI(TAG, "BLE advertising");
        }
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

    case ESP_GATTS_CONNECT_EVT: {
        /* The controller stops advertising by itself on every connection. */
        s_advertising = false;

        conn_t *c = conn_alloc(param->connect.conn_id);
        if (!c) {
            esp_ble_gatts_close(gatts_if, param->connect.conn_id);
            break;
        }
        memcpy(c->bda, param->connect.remote_bda, sizeof(esp_bd_addr_t));
        ESP_LOGI(TAG, "BLE client connected (%d of %d)", conn_count(), MAX_CONNS);

        if (conn_count() < MAX_CONNS) start_adv_when_ready();
        break;
    }

    case ESP_GATTS_DISCONNECT_EVT: {
        conn_t *c = conn_find(param->disconnect.conn_id);
        if (c) memset(c, 0, sizeof(*c));
        fw_update_disconnected(param->disconnect.conn_id);
        ESP_LOGI(TAG, "BLE client disconnected (%d of %d left)", conn_count(), MAX_CONNS);
        start_adv_when_ready();
        break;
    }

    case ESP_GATTS_MTU_EVT: {
        conn_t *c = conn_find(param->mtu.conn_id);
        if (c) c->mtu = param->mtu.mtu;
        ESP_LOGI(TAG, "MTU %u", param->mtu.mtu);
        break;
    }

    case ESP_GATTS_WRITE_EVT: {
        const uint16_t h = param->write.handle;
        conn_t *c = conn_find(param->write.conn_id);

        if (h == s_handles[IDX_CMD_VAL]) {
            handle_command(param->write.value, param->write.len);
        } else if (h == s_handles[IDX_FWD_VAL]) {
            fw_update_data(param->write.conn_id, param->write.value, param->write.len);
            if (param->write.need_rsp)
                esp_ble_gatts_send_response(gatts_if, param->write.conn_id,
                                            param->write.trans_id, ESP_GATT_OK, NULL);
        } else if (h == s_handles[IDX_FWC_VAL]) {
            fw_update_ctrl(param->write.conn_id, param->write.value, param->write.len);
        } else if (h == s_handles[IDX_CFG_CCC] && param->write.len == 2 && c) {
            c->cfg_notify = (param->write.value[0] & 0x01) != 0;
            ESP_LOGI(TAG, "notifications %s", c->cfg_notify ? "on" : "off");
        } else if (h == s_handles[IDX_FWC_CCC] && param->write.len == 2 && c) {
            c->fw_notify = (param->write.value[0] & 0x01) != 0;
        }
        break;
    }

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

/* ------------------------------------------------------- firmware update */

void ble_ctl_fw_event(uint16_t conn_id, const uint8_t *data, uint16_t len)
{
    const conn_t *c = conn_find(conn_id);
    if (!c || !c->fw_notify) return;
    esp_ble_gatts_send_indicate(s_gatts_if, conn_id, s_handles[IDX_FWC_VAL],
                                len, (uint8_t *)data, false);
}

void ble_ctl_fast_link(uint16_t conn_id)
{
    conn_t *c = conn_find(conn_id);
    if (!c) return;

    /* 15-30 ms is the shortest interval Apple hosts will grant, and with
     * 251-byte link packets each 244-byte write goes out as a single PDU. */
    esp_ble_conn_update_params_t p = {
        .min_int = 0x0C, .max_int = 0x18, .latency = 0, .timeout = 500,
    };
    memcpy(p.bda, c->bda, sizeof(esp_bd_addr_t));
    esp_ble_gap_update_conn_params(&p);
    esp_ble_gap_set_pkt_data_len(c->bda, 251);
}
