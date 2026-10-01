#include "chime.h"

#include <math.h>
#include <string.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/* ------------------------------------------------------------ oscillator */

/*
 * A table-driven oscillator, not sinf() per sample. Four overlapping voices
 * with three harmonics each is twelve sines per frame; calling sinf() that
 * often at 44.1 kHz would eat a serious fraction of the CPU the DSP needs,
 * and an underrun during the startup tone is exactly the wrong first
 * impression. Linear interpolation over 1024 points is inaudible here.
 */
#define SINTAB_N 1024
static float s_sintab[SINTAB_N + 1];
static bool  s_tab_ready;

static void sintab_init(void)
{
    if (s_tab_ready) return;
    for (int i = 0; i <= SINTAB_N; i++)
        s_sintab[i] = sinf(2.0f * (float)M_PI * (float)i / (float)SINTAB_N);
    s_tab_ready = true;
}

/* Phase is Q32 across one period, so harmonics are just phase * n. */
static inline float osc(uint32_t phase)
{
    uint32_t idx  = phase >> 22;                                  /* top 10 bits */
    float    frac = (float)((phase >> 6) & 0xFFFF) * (1.0f / 65536.0f);
    return s_sintab[idx] + (s_sintab[idx + 1] - s_sintab[idx]) * frac;
}

/* ---------------------------------------------------------------- voices */

#define MAX_VOICES 4

typedef struct {
    float    freq;
    uint16_t delay_ms;     /* start offset within the tone */
    uint16_t ring_ms;      /* how long it rings */
    float    amp;
} note_t;

/* Simple triads: rising to greet, falling to say goodbye. */
static const note_t STARTUP[] = {
    { 523.25f,   0, 520, 0.16f },   /* C5 */
    { 659.25f,  90, 480, 0.15f },   /* E5 */
    { 783.99f, 180, 460, 0.15f },   /* G5 */
    { 1046.50f, 270, 620, 0.17f },  /* C6 */
};
static const note_t CONNECT[] = {
    { 783.99f,   0, 260, 0.20f },   /* G5 */
    { 1046.50f, 85, 400, 0.22f },   /* C6 */
};
static const note_t DISCONNECT[] = {
    { 1046.50f,  0, 260, 0.20f },   /* C6 */
    { 783.99f,  85, 400, 0.22f },   /* G5 */
};

typedef struct {
    uint32_t phase, inc;
    uint32_t start, len;   /* in samples */
    float    amp, env, decay;
    float    attack_inc;
    uint32_t attack;
} voice_t;

static struct {
    voice_t  v[MAX_VOICES];
    uint8_t  n;
    uint32_t pos, total;
    volatile bool active;
} s;

void chime_start(chime_id_t id, uint32_t fs)
{
    if (!fs) fs = 44100;
    sintab_init();

    const note_t *notes;
    uint8_t n;
    switch (id) {
    case CHIME_CONNECT:    notes = CONNECT;    n = 2; break;
    case CHIME_DISCONNECT: notes = DISCONNECT; n = 2; break;
    default:               notes = STARTUP;    n = 4; break;
    }

    memset(&s, 0, sizeof(s));
    s.n = n;

    const float attack_ms = 6.0f;   /* long enough to avoid a click */

    for (uint8_t i = 0; i < n; i++) {
        voice_t *v = &s.v[i];
        v->inc   = (uint32_t)((double)notes[i].freq / (double)fs * 4294967296.0);
        v->start = (uint32_t)((uint64_t)notes[i].delay_ms * fs / 1000);
        v->len   = (uint32_t)((uint64_t)notes[i].ring_ms  * fs / 1000);
        v->amp   = notes[i].amp;
        v->env   = 0.0f;

        v->attack     = (uint32_t)(attack_ms * fs / 1000.0f);
        v->attack_inc = v->attack ? 1.0f / (float)v->attack : 1.0f;

        /* Exponential ring-out reaching about -60 dB by the end of the note. */
        float tail = (float)(v->len > v->attack ? v->len - v->attack : 1);
        v->decay = expf(-6.9078f / tail);

        uint32_t end = v->start + v->len;
        if (end > s.total) s.total = end;
    }

    s.pos = 0;
    s.active = true;
}

bool chime_active(void) { return s.active; }

void chime_render(int16_t *out, size_t frames)
{
    if (!s.active) { memset(out, 0, frames * 2 * sizeof(int16_t)); return; }

    for (size_t i = 0; i < frames; i++) {
        float acc = 0.0f;

        for (uint8_t k = 0; k < s.n; k++) {
            voice_t *v = &s.v[k];
            if (s.pos < v->start || s.pos >= v->start + v->len) continue;

            uint32_t t = s.pos - v->start;
            if (t < v->attack) v->env += v->attack_inc;
            else               v->env *= v->decay;

            /* A little 2nd and 3rd harmonic stops it sounding like a test tone. */
            float y = osc(v->phase)
                    + 0.22f * osc(v->phase * 2u)
                    + 0.09f * osc(v->phase * 3u);
            acc += v->amp * v->env * y;

            v->phase += v->inc;
        }

        if (acc >  1.0f) acc =  1.0f;
        if (acc < -1.0f) acc = -1.0f;

        int16_t sample = (int16_t)(acc * 30000.0f);
        out[i * 2]     = sample;
        out[i * 2 + 1] = sample;

        if (++s.pos >= s.total) {
            /* Pad the rest of the block and stop. */
            for (size_t j = i + 1; j < frames; j++) {
                out[j * 2] = 0; out[j * 2 + 1] = 0;
            }
            s.active = false;
            return;
        }
    }
}
