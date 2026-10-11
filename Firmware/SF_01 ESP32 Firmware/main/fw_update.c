#include "fw_update.h"
#include "ble_ctl.h"
#include "audio_out.h"

#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/ringbuf.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_timer.h"
#include "esp_ota_ops.h"
#include "esp_app_desc.h"
#include "esp_check.h"

static const char *TAG = "fw_update";

enum { CMD_BEGIN = 0x01, CMD_ABORT = 0x03, CMD_CONFIRM = 0x04 };
enum { EVT_READY = 0x01, EVT_ACK = 0x02, EVT_DONE = 0x03, EVT_CONFIRMED = 0x04, EVT_ERROR = 0x7F };
enum { ST_IDLE = 0, ST_RECEIVING, ST_VERIFYING, ST_REBOOTING };

/* The client may run this far ahead of what has reached flash. */
#define WINDOW_BYTES     8192
#define RING_BYTES       (WINDOW_BYTES + 2048)
#define ACK_EVERY        2048
#define DATA_TIMEOUT_MS  15000

static SemaphoreHandle_t s_lock;       /* guards s_state + s_rb against teardown */
static RingbufHandle_t   s_rb;
static volatile uint8_t  s_state = ST_IDLE;
static volatile uint8_t  s_abort;      /* FW_ERR_* to stop with, 0 = keep going */
static uint16_t          s_owner;      /* connection that started the session  */
static uint32_t          s_size;
static uint32_t          s_received;   /* accepted from BLE                    */
static volatile uint32_t s_written;    /* written to flash                     */
static bool              s_restore_unmute;

static bool               s_pending;   /* running image is on probation */
static esp_timer_handle_t s_rollback_timer;

static inline void put_u32(uint8_t *p, uint32_t v)
{
    p[0] = v & 0xFF; p[1] = (v >> 8) & 0xFF; p[2] = (v >> 16) & 0xFF; p[3] = v >> 24;
}

static void notify_u32(uint8_t evt, uint32_t v)
{
    uint8_t f[5] = { evt };
    put_u32(f + 1, v);
    ble_ctl_fw_event(s_owner, f, sizeof(f));
}

static void notify_to(uint16_t conn, uint8_t evt, uint8_t arg, uint16_t len)
{
    uint8_t f[2] = { evt, arg };
    ble_ctl_fw_event(conn, f, len);
}

/* --------------------------------------------------------------- transfer */

static void update_task(void *arg)
{
    esp_ota_handle_t handle = 0;
    bool      open = false;
    uint8_t   fail = 0;
    esp_err_t err  = ESP_OK;
    uint32_t  last_ack = 0, idle_ms = 0;

    const esp_partition_t *part = esp_ota_get_next_update_partition(NULL);
    ESP_LOGI(TAG, "update started: %lu bytes -> %s", (unsigned long)s_size, part->label);

    /* Sequential mode erases as it writes, so the erase cost is spread over
     * the transfer instead of stalling the link for ten seconds up front. */
    err = esp_ota_begin(part, OTA_WITH_SEQUENTIAL_WRITES, &handle);
    if (err != ESP_OK) { fail = FW_ERR_BEGIN; goto failed; }
    open = true;

    notify_u32(EVT_READY, WINDOW_BYTES);

    while (s_written < s_size) {
        if (s_abort) { fail = s_abort; goto failed; }

        size_t got = 0;
        uint8_t *chunk = xRingbufferReceiveUpTo(s_rb, &got, pdMS_TO_TICKS(250), 4096);
        if (!chunk) {
            idle_ms += 250;
            if (idle_ms >= DATA_TIMEOUT_MS) { fail = FW_ERR_TIMEOUT; goto failed; }
            /* If an ACK went missing the client is waiting on window while
             * we wait on data. Repeating the count once a second breaks it. */
            if (idle_ms % 1000 == 0) notify_u32(EVT_ACK, s_written);
            continue;
        }
        idle_ms = 0;

        err = esp_ota_write(handle, chunk, got);
        vRingbufferReturnItem(s_rb, chunk);
        if (err != ESP_OK) { fail = FW_ERR_WRITE; goto failed; }

        s_written += got;
        if (s_written - last_ack >= ACK_EVERY || s_written == s_size) {
            last_ack = s_written;
            notify_u32(EVT_ACK, s_written);
        }
    }

    /* esp_ota_end() checks the image hash and, because signed updates are
     * required, its ECDSA signature against the key in this firmware. */
    s_state = ST_VERIFYING;
    err = esp_ota_end(handle);
    open = false;
    if (err != ESP_OK) { fail = FW_ERR_VERIFY; goto failed; }

    err = esp_ota_set_boot_partition(part);
    if (err != ESP_OK) { fail = FW_ERR_SET_BOOT; goto failed; }

    s_state = ST_REBOOTING;
    notify_to(s_owner, EVT_DONE, 0, 1);
    ESP_LOGI(TAG, "image verified; rebooting into %s", part->label);

    /* Long enough for the notification to leave and the amps to be quiet. */
    vTaskDelay(pdMS_TO_TICKS(1200));
    esp_restart();

failed:
    if (open) esp_ota_abort(handle);
    ESP_LOGE(TAG, "update failed: code %u (%s) after %lu of %lu bytes",
             fail, esp_err_to_name(err), (unsigned long)s_written, (unsigned long)s_size);
    notify_to(s_owner, EVT_ERROR, fail, 2);

    xSemaphoreTake(s_lock, portMAX_DELAY);
    RingbufHandle_t rb = s_rb;
    s_rb = NULL;
    s_state = ST_IDLE;
    xSemaphoreGive(s_lock);
    if (rb) vRingbufferDelete(rb);

    if (s_restore_unmute) audio_out_unmute();
    vTaskDelete(NULL);
}

static void begin(uint16_t conn, const uint8_t *d, uint16_t len)
{
    if (len < 5) { notify_to(conn, EVT_ERROR, FW_ERR_BAD_REQUEST, 2); return; }
    uint32_t size = (uint32_t)d[1] | ((uint32_t)d[2] << 8) |
                    ((uint32_t)d[3] << 16) | ((uint32_t)d[4] << 24);

    const esp_partition_t *part = esp_ota_get_next_update_partition(NULL);
    if (!part || size == 0 || size > part->size) {
        notify_to(conn, EVT_ERROR, FW_ERR_TOO_LARGE, 2);
        return;
    }

    xSemaphoreTake(s_lock, portMAX_DELAY);
    if (s_state != ST_IDLE) {
        xSemaphoreGive(s_lock);
        notify_to(conn, EVT_ERROR, FW_ERR_BUSY, 2);
        return;
    }
    s_rb = xRingbufferCreate(RING_BYTES, RINGBUF_TYPE_BYTEBUF);
    if (!s_rb) {
        xSemaphoreGive(s_lock);
        notify_to(conn, EVT_ERROR, FW_ERR_BEGIN, 2);
        return;
    }
    s_owner = conn;
    s_size = size;
    s_received = 0;
    s_written = 0;
    s_abort = 0;
    s_state = ST_RECEIVING;
    xSemaphoreGive(s_lock);

    /* Flash writes stall every task that runs from flash for milliseconds at
     * a time, which is audible. Go quiet for the duration. */
    s_restore_unmute = audio_out_is_unmuted();
    audio_out_mute();
    ble_ctl_fast_link(conn);

    /* Generous stack: esp_ota_end() does the SHA-256 and ECDSA check here. */
    if (xTaskCreate(update_task, "fw_update", 8192, NULL, 6, NULL) != pdPASS) {
        xSemaphoreTake(s_lock, portMAX_DELAY);
        vRingbufferDelete(s_rb);
        s_rb = NULL;
        s_state = ST_IDLE;
        xSemaphoreGive(s_lock);
        if (s_restore_unmute) audio_out_unmute();
        notify_to(conn, EVT_ERROR, FW_ERR_BEGIN, 2);
    }
}

void fw_update_ctrl(uint16_t conn, const uint8_t *d, uint16_t len)
{
    if (!len) return;
    switch (d[0]) {
    case CMD_BEGIN:
        begin(conn, d, len);
        break;
    case CMD_ABORT:
        if (s_state == ST_RECEIVING && conn == s_owner) s_abort = FW_ERR_ABORTED;
        break;
    case CMD_CONFIRM:
        fw_update_confirm();
        notify_to(conn, EVT_CONFIRMED, 0, 1);
        break;
    default:
        notify_to(conn, EVT_ERROR, FW_ERR_BAD_REQUEST, 2);
        break;
    }
}

/* Runs on the Bluetooth stack task: queue the bytes and get out. */
void fw_update_data(uint16_t conn, const uint8_t *d, uint16_t len)
{
    if (!len) return;
    xSemaphoreTake(s_lock, portMAX_DELAY);
    if (s_state == ST_RECEIVING && conn == s_owner && !s_abort) {
        if (s_received + len > s_size)
            s_abort = FW_ERR_BAD_REQUEST;
        else if (xRingbufferSend(s_rb, d, len, 0) != pdTRUE)
            s_abort = FW_ERR_OVERFLOW;       /* client ignored the window */
        else
            s_received += len;
    }
    xSemaphoreGive(s_lock);
}

void fw_update_disconnected(uint16_t conn)
{
    if (s_state == ST_RECEIVING && conn == s_owner) s_abort = FW_ERR_ABORTED;
}

/* --------------------------------------------------------------- probation */

static void rollback_cb(void *arg)
{
    ESP_LOGE(TAG, "new firmware was not confirmed within %d s; rolling back",
             FW_CONFIRM_TIMEOUT_S);
    audio_out_mute();
    vTaskDelay(pdMS_TO_TICKS(200));
    esp_ota_mark_app_invalid_rollback_and_reboot();
    ESP_LOGE(TAG, "rollback was not possible; staying on this image");
}

bool fw_update_confirm(void)
{
    if (!s_pending) return false;
    if (esp_ota_mark_app_valid_cancel_rollback() != ESP_OK) return false;
    s_pending = false;
    if (s_rollback_timer) esp_timer_stop(s_rollback_timer);
    ESP_LOGI(TAG, "new firmware confirmed; it is now the permanent image");
    return true;
}

esp_err_t fw_update_init(void)
{
    s_lock = xSemaphoreCreateMutex();
    if (!s_lock) return ESP_ERR_NO_MEM;

    const esp_partition_t *run = esp_ota_get_running_partition();
    const esp_app_desc_t  *app = esp_app_get_description();
    ESP_LOGI(TAG, "firmware %s (%s %s) running from %s",
             app->version, app->date, app->time, run ? run->label : "?");

    esp_ota_img_states_t st;
    if (run && esp_ota_get_state_partition(run, &st) == ESP_OK &&
        st == ESP_OTA_IMG_PENDING_VERIFY) {
        s_pending = true;
        const esp_timer_create_args_t t = { .callback = rollback_cb, .name = "fw_rollback" };
        ESP_RETURN_ON_ERROR(esp_timer_create(&t, &s_rollback_timer), TAG, "timer");
        ESP_RETURN_ON_ERROR(esp_timer_start_once(s_rollback_timer,
                                (uint64_t)FW_CONFIRM_TIMEOUT_S * 1000000ULL), TAG, "timer");
        ESP_LOGW(TAG, "this image is on probation: rolls back in %d s unless confirmed over BLE",
                 FW_CONFIRM_TIMEOUT_S);
    }
    return ESP_OK;
}

void fw_update_info(uint8_t out[FW_INFO_LEN])
{
    const esp_partition_t *run = esp_ota_get_running_partition();
    const esp_app_desc_t  *app = esp_app_get_description();

    memset(out, 0, FW_INFO_LEN);
    out[0] = 1;
    out[1] = s_state;
    out[2] = s_pending ? 0x01 : 0;
    out[3] = (run && run->subtype >= ESP_PARTITION_SUBTYPE_APP_OTA_MIN)
           ? (uint8_t)(run->subtype - ESP_PARTITION_SUBTYPE_APP_OTA_MIN) : 0xFF;
    put_u32(out + 4, s_written);
    memcpy(out + 8, app->app_elf_sha256, 8);
    strncpy((char *)out + 16, app->version, 31);
    strncpy((char *)out + 48, app->date, 15);
    strncpy((char *)out + 64, app->time, 15);
}
