/*
 * dsp.h — 2-way crossover + parametric EQ for the SF_01.
 *
 * Signal chain, per stereo frame:
 *
 *   in ──► volume ──► EQ (N bands) ──┬──► LR4 low-pass  ──► woofer trim  ──► woofer I2S
 *                                    └──► LR4 high-pass ──► tweeter trim ──► tweeter I2S
 *                                              (+ optional polarity / delay)
 *
 * The crossover is Linkwitz-Riley 4th order: two cascaded Butterworth
 * (Q = 0.707) sections per branch. LR4 low and high branches sum to an
 * all-pass, so the drivers stay in phase through the crossover region
 * and no polarity flip is needed — unlike LR2.
 */
#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#define DSP_MAX_EQ_BANDS      8
#define DSP_MAX_DELAY_SAMPLES 64

typedef enum {
    EQ_OFF = 0,
    EQ_PEAKING,
    EQ_LOWSHELF,
    EQ_HIGHSHELF,
} eq_type_t;

typedef struct {
    eq_type_t type;
    float     freq_hz;
    float     q;
    float     gain_db;
} eq_band_t;

typedef struct {
    float    crossover_hz;       /* LR4 crossover frequency            */
    float    woofer_gain_db;     /* per-way sensitivity trim           */
    float    tweeter_gain_db;
    float    master_gain_db;     /* fixed headroom / calibration trim  */
    uint8_t  volume;             /* 0..127, tracks AVRCP absolute vol  */
    bool     tweeter_invert;     /* polarity flip for crossover tuning */
    uint16_t tweeter_delay;      /* integer-sample time align, 0..64   */
    eq_band_t eq[DSP_MAX_EQ_BANDS];
} dsp_config_t;

/* Fill cfg with the default tuning for the SF_01 driver complement. */
void dsp_default_config(dsp_config_t *cfg);

/* Must be called before dsp_process(). Safe to call again on rate change. */
void dsp_init(uint32_t sample_rate);

/* Publish a new configuration. Thread-safe; coefficients are recomputed
 * by the audio task at the next block boundary, never mid-block. */
void dsp_set_config(const dsp_config_t *cfg);
void dsp_get_config(dsp_config_t *cfg);

uint32_t dsp_get_sample_rate(void);

/*
 * Process one block.
 *   in       interleaved stereo int16, `frames` frames
 *   woofer   interleaved stereo int16 out, `frames` frames
 *   tweeter  interleaved stereo int16 out, `frames` frames
 * in may alias neither output.
 */
void dsp_process(const int16_t *in, int16_t *woofer, int16_t *tweeter, size_t frames);

/*
 * Envelope of the low bass, 0.0 .. ~1.0, for the LED visualiser.
 *
 * Tapped off the woofer branch after its gain, so it follows what the drivers
 * actually receive — volume and EQ included — rather than the raw stream.
 * Band-limited to ~120 Hz, because the woofer branch itself runs all the way
 * to the crossover and lighting an LED off that just tracks overall loudness.
 */
float dsp_bass_level(void);
