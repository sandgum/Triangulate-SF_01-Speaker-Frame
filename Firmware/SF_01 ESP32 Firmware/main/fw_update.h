/*
 * fw_update.h — over-the-air firmware updates, carried on the BLE control link.
 *
 * Three more characteristics on the SF_01 control service (see ble_ctl.h):
 *
 *   FW_CTRL  write | notify       5F010003-A1B2-4C3D-8E4F-5F0100000001
 *   FW_DATA  write-without-rsp    5F010004-A1B2-4C3D-8E4F-5F0100000001
 *   FW_INFO  read                 5F010005-A1B2-4C3D-8E4F-5F0100000001
 *
 * ------------------------------------------------------------------ session
 *
 *   client                                   speaker
 *   FW_CTRL  BEGIN  size ───────────────────►  opens the idle app slot
 *                         ◄─────────────────── READY  window
 *   FW_DATA  raw image bytes ───────────────►  ring buffer ─► flash
 *                         ◄─────────────────── ACK  bytes written (every 2 kB)
 *                         ◄─────────────────── DONE   image verified, rebooting
 *   …reconnect after the reboot…
 *   FW_INFO  read: is it the build just sent?
 *   FW_CTRL  CONFIRM ───────────────────────►  keeps the new image for good
 *
 * Flow control: FW_DATA is unacknowledged at the GATT layer, so the client
 * may never have more than `window` bytes outstanding beyond the last ACK.
 * ACK counts bytes actually written to flash, which makes the window an
 * exact bound on what the speaker has to buffer.
 *
 * FW_CTRL commands (little-endian):
 *   0x01 BEGIN    u32 image size
 *   0x03 ABORT
 *   0x04 CONFIRM
 *
 * FW_CTRL notifications:
 *   0x01 READY      u32 window
 *   0x02 ACK        u32 bytes written
 *   0x03 DONE
 *   0x04 CONFIRMED
 *   0x7F ERROR      u8 code (FW_ERR_*)
 *
 * FW_INFO, 80 bytes:
 *    0  1   layout version (= 1)
 *    1  1   state: 0 idle, 1 receiving, 2 verifying, 3 rebooting
 *    2  1   flags: bit0 running image is unconfirmed (on probation)
 *    3  1   running slot (0 = ota_0, 1 = ota_1)
 *    4  4   bytes written in the current transfer
 *    8  8   first 8 bytes of the running image's ELF SHA-256 — the build id
 *   16 32   version string
 *   48 16   build date
 *   64 16   build time
 *
 * ------------------------------------------------------------------- safety
 *
 * Two things stop a bad push from stranding the speaker until someone finds
 * a USB cable:
 *
 *  - Images are signature-checked (ECDSA) before they are accepted, against
 *    the public key built into the running firmware. Anyone in Bluetooth
 *    range can open this service; only someone holding the signing key can
 *    get an image past esp_ota_end().
 *
 *  - A new image boots on probation. It is kept only once a client has
 *    reconnected over BLE and sent CONFIRM — which proves the new firmware
 *    can still be updated. If that does not happen within
 *    FW_CONFIRM_TIMEOUT_S, or the speaker resets first, the bootloader goes
 *    back to the previous image.
 */
#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"

#define FW_INFO_LEN           80
#define FW_CONFIRM_TIMEOUT_S  180

enum {
    FW_ERR_BAD_REQUEST = 1,
    FW_ERR_TOO_LARGE   = 2,
    FW_ERR_BEGIN       = 3,
    FW_ERR_OVERFLOW    = 4,
    FW_ERR_WRITE       = 5,
    FW_ERR_TIMEOUT     = 6,
    FW_ERR_VERIFY      = 7,
    FW_ERR_SET_BOOT    = 8,
    FW_ERR_BUSY        = 9,
    FW_ERR_ABORTED     = 10,
};

/* Call once at boot. Arms the rollback timer if the running image is on probation. */
esp_err_t fw_update_init(void);

/* Called by the GATT server with writes to FW_CTRL / FW_DATA. */
void fw_update_ctrl(uint16_t conn_id, const uint8_t *data, uint16_t len);
void fw_update_data(uint16_t conn_id, const uint8_t *data, uint16_t len);
void fw_update_disconnected(uint16_t conn_id);

/* Current contents of FW_INFO. */
void fw_update_info(uint8_t out[FW_INFO_LEN]);

/* Keep the running image (same effect as the CONFIRM command). */
bool fw_update_confirm(void);
