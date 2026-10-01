/*
 * audio_out.h — dual PCM5102A output stage and mute/enable sequencing.
 */
#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "esp_err.h"
#include "chime.h"

/*
 * Put the analog chain in its safe state. Call this as the FIRST thing in
 * app_main, before NVS, Bluetooth or anything else that can block:
 * until it runs, DRIVER_MUTE is held low by the 5 x 100 k pull-down network
 * on the board and the amplifiers are live with an undefined DAC output.
 */
void audio_out_safe_state(void);

/* Create both I2S channels, start clocks, then run the power-up sequence. */
esp_err_t audio_out_init(uint32_t sample_rate);

/* Un-mute (DAC first, then amps) / mute (amps first, then DAC). */
void audio_out_unmute(void);
void audio_out_mute(void);

/* Re-clock both DACs for a new A2DP sample rate. Mutes around the change. */
esp_err_t audio_out_set_rate(uint32_t sample_rate);

/* Feed decoded interleaved stereo int16 from the A2DP callback. */
void audio_out_submit(const uint8_t *pcm, size_t bytes);

bool audio_out_is_unmuted(void);

/* Queue a UI tone. Un-mutes if needed and restores the previous mute state. */
void audio_out_play_chime(chime_id_t id);
