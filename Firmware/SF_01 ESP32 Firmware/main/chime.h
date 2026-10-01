/*
 * chime.h — synthesised UI tones (startup, connect, disconnect).
 *
 * The tones are generated, not stored: no flash budget, no WAV assets, and
 * they follow whatever sample rate A2DP negotiated without resampling.
 *
 * Rendering happens inside the audio render task, in place of the stream,
 * rather than being pushed through the ring buffer. That means no extra
 * buffering, no pacing logic — the I2S clock paces it — and the tone still
 * runs through the normal DSP chain, so the crossover splits it across the
 * woofers and tweeters properly instead of landing on one way only.
 */
#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

typedef enum {
    CHIME_STARTUP = 0,
    CHIME_CONNECT,
    CHIME_DISCONNECT,
} chime_id_t;

/* Arm a tone. Safe to call from any task; rendering happens elsewhere. */
void   chime_start(chime_id_t id, uint32_t sample_rate);

bool   chime_active(void);

/* Fill `frames` of interleaved stereo. Pads with silence once the tone
 * finishes and clears the active flag. */
void   chime_render(int16_t *out, size_t frames);
