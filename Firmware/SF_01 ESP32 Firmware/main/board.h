/*
 * board.h — SF_01 Compute Board pin map.
 *
 * Pin numbers are taken directly from the KiCad netlist of
 * "PCB V1.1/SF_01 Compute Board", module U28 (ESP32-WROVER-E-N16R8).
 * The bring-up devboard uses the same GPIO numbers, wired to the
 * matching test points, so one binary runs on both.
 *
 *   signal        GPIO   U28 pin   test point
 *   WFR_BCK        25      10        TP11
 *   WFR_LRCK       19      31        TP18
 *   WFR_DIN        23      37        TP15
 *   WFR_SCK        26      11        TP12
 *   TWT_BCK        15      23        TP8
 *   TWT_LRCK       13      16        TP9
 *   TWT_DIN        14      13        TP13
 *   TWT_SCK        18      30        TP16
 *   DAC_EN          4      26        TP17
 *   DRIVER_MUTE    27      12        TP14
 */
#pragma once

#include "driver/gpio.h"

/* Woofer / mid-bass DAC (U14) */
#define PIN_WFR_BCK      GPIO_NUM_25
#define PIN_WFR_LRCK     GPIO_NUM_19
#define PIN_WFR_DIN      GPIO_NUM_23
#define PIN_WFR_SCK      GPIO_NUM_26

/* Tweeter DAC (U5) */
#define PIN_TWT_BCK      GPIO_NUM_15
#define PIN_TWT_LRCK     GPIO_NUM_13
#define PIN_TWT_DIN      GPIO_NUM_14
#define PIN_TWT_SCK      GPIO_NUM_18

/*
 * DAC_EN drives XSMT (pin 17) on BOTH PCM5102As in parallel.
 * PCM5102A: XSMT low = soft mute, high = soft un-mute.
 */
#define PIN_DAC_EN       GPIO_NUM_4
#define DAC_UNMUTED      1
#define DAC_MUTED        0

/*
 * DRIVER_MUTE drives MUTE (pin 12) on all five amplifiers
 * (U17/U18/U19 TPA3116D2, U20/U21 TPA3130D2) through the 100 k
 * series resistors TI requires when the driver slews faster than
 * 10 V/ms (SLOS708G abs-max table, note 2).
 * TPA31xx: MUTE high = outputs Hi-Z, low = outputs enabled.
 */
#define PIN_DRIVER_MUTE  GPIO_NUM_27
#define AMPS_MUTED       1
#define AMPS_UNMUTED     0

/* I2C sensors (BME680, MPU-6050) — not used by the audio path yet. */
#define PIN_I2C_SDA      GPIO_NUM_21
#define PIN_I2C_SCL      GPIO_NUM_22
