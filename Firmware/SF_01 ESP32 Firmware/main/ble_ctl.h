/*
 * ble_ctl.h — BLE GATT server for remote EQ / crossover control.
 *
 * Runs alongside the A2DP sink: the controller is in dual mode (BTDM), so
 * BR/EDR carries the audio while BLE carries control. The two share one
 * radio via the coexistence manager — control traffic is occasional and
 * small, which is exactly the case that coexists cleanly with A2DP.
 *
 * ---------------------------------------------------------------- protocol
 *
 * Service                              5F010000-A1B2-4C3D-8E4F-5F0100000001
 *   CONFIG   read | notify             5F010001-A1B2-4C3D-8E4F-5F0100000001
 *   COMMAND  write | write-no-rsp      5F010002-A1B2-4C3D-8E4F-5F0100000001
 *
 * CONFIG is a 78-byte little-endian snapshot of the whole DSP state. It is
 * republished (and notified) whenever anything changes it, including the
 * UART shell and AVRCP volume, so the phone never shows a stale value.
 *
 *   off  size  field
 *    0    1    version (= SF01_BLE_PROTO_VERSION)
 *    1    1    flags: bit0 tweeter_invert, bit1 output_live
 *    2    2    crossover_hz               u16
 *    4    1    volume 0..127              u8
 *    5    1    tweeter_delay samples      u8
 *    6    2    woofer_gain_db  x10        i16
 *    8    2    tweeter_gain_db x10        i16
 *   10    2    master_gain_db  x10        i16
 *   12    2    sample_rate / 100          u16
 *   14   64    8 bands x 8 bytes:
 *                0 1 type (0 off, 1 peak, 2 lowshelf, 3 highshelf)
 *                1 1 reserved
 *                2 2 freq_hz              u16
 *                4 2 Q x100               u16
 *                6 2 gain_db x10          i16
 *
 * COMMAND is a short opcode frame, at most 9 bytes, so it fits even at the
 * 23-byte default ATT MTU:
 *
 *   0x01 SET_XO      u16 hz
 *   0x02 SET_VOLUME  u8 0..127
 *   0x03 SET_GAIN    u8 target (0 woofer, 1 tweeter, 2 master), i16 dB x10
 *   0x04 SET_EQ      u8 band, u8 type, u16 freq, u16 Q x100, i16 dB x10
 *   0x05 SET_INVERT  u8 0|1
 *   0x06 SET_DELAY   u8 samples
 *   0x07 SAVE        -
 *   0x08 LOAD        -
 *   0x09 DEFAULTS    -
 *   0x0A SET_MUTE    u8 0 un-mute, 1 mute
 */
#pragma once

#include "esp_err.h"

#define SF01_BLE_PROTO_VERSION 1
#define SF01_BLE_CONFIG_LEN    78

/* Call after bt_sink_start(); the controller must already be up in BTDM. */
esp_err_t ble_ctl_start(void);
