#include "shell.h"
#include "dsp.h"
#include "settings.h"
#include "audio_out.h"
#include "led_viz.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "esp_console.h"
#include "linenoise/linenoise.h"
#include "esp_log.h"

static const char *type_name(eq_type_t t)
{
    switch (t) {
    case EQ_PEAKING:   return "peak";
    case EQ_LOWSHELF:  return "lowshelf";
    case EQ_HIGHSHELF: return "highshelf";
    default:           return "off";
    }
}

static eq_type_t type_parse(const char *s)
{
    if (!strcmp(s, "peak"))      return EQ_PEAKING;
    if (!strcmp(s, "lowshelf"))  return EQ_LOWSHELF;
    if (!strcmp(s, "highshelf")) return EQ_HIGHSHELF;
    return EQ_OFF;
}

static int cmd_status(int argc, char **argv)
{
    dsp_config_t c;
    dsp_get_config(&c);

    printf("\n  sample rate    %lu Hz\n", (unsigned long)dsp_get_sample_rate());
    printf("  crossover      %.0f Hz  (Linkwitz-Riley 24 dB/oct)\n", c.crossover_hz);
    printf("  woofer trim    %+.1f dB\n", c.woofer_gain_db);
    printf("  tweeter trim   %+.1f dB%s\n", c.tweeter_gain_db,
           c.tweeter_invert ? "  (polarity inverted)" : "");
    printf("  master trim    %+.1f dB\n", c.master_gain_db);
    printf("  volume         %u/127\n", c.volume);
    printf("  tweeter delay  %u samples\n", c.tweeter_delay);
    printf("  output         %s\n", audio_out_is_unmuted() ? "live" : "muted");
    printf("\n  band  type       freq      Q     gain\n");
    for (int i = 0; i < DSP_MAX_EQ_BANDS; i++) {
        const eq_band_t *b = &c.eq[i];
        printf("   %d    %-9s %6.0f Hz %5.2f  %+.1f dB\n",
               i, type_name(b->type), b->freq_hz, b->q, b->gain_db);
    }
    printf("\n");
    return 0;
}

static int cmd_vol(int argc, char **argv)
{
    if (argc < 2) { printf("usage: vol <0-127>\n"); return 1; }
    dsp_config_t c; dsp_get_config(&c);
    c.volume = (uint8_t)atoi(argv[1]);
    dsp_set_config(&c);
    printf("volume %u/127\n", c.volume);
    return 0;
}

static int cmd_xo(int argc, char **argv)
{
    if (argc < 2) { printf("usage: xo <hz>\n"); return 1; }
    dsp_config_t c; dsp_get_config(&c);
    c.crossover_hz = strtof(argv[1], NULL);
    dsp_set_config(&c);
    printf("crossover %.0f Hz\n", c.crossover_hz);
    return 0;
}

static int cmd_eq(int argc, char **argv)
{
    if (argc < 6) {
        printf("usage: eq <band 0-%d> <off|peak|lowshelf|highshelf> "
               "<freq_hz> <Q> <gain_db>\n", DSP_MAX_EQ_BANDS - 1);
        return 1;
    }
    int idx = atoi(argv[1]);
    if (idx < 0 || idx >= DSP_MAX_EQ_BANDS) { printf("band out of range\n"); return 1; }

    dsp_config_t c; dsp_get_config(&c);
    c.eq[idx].type    = type_parse(argv[2]);
    c.eq[idx].freq_hz = strtof(argv[3], NULL);
    c.eq[idx].q       = strtof(argv[4], NULL);
    c.eq[idx].gain_db = strtof(argv[5], NULL);
    dsp_set_config(&c);

    printf("band %d: %s %.0f Hz Q=%.2f %+.1f dB\n", idx,
           type_name(c.eq[idx].type), c.eq[idx].freq_hz,
           c.eq[idx].q, c.eq[idx].gain_db);
    return 0;
}

static int cmd_gain(int argc, char **argv)
{
    if (argc < 3) { printf("usage: gain <wfr|twt|master> <dB>\n"); return 1; }
    dsp_config_t c; dsp_get_config(&c);
    float v = strtof(argv[2], NULL);

    if      (!strcmp(argv[1], "wfr"))    c.woofer_gain_db  = v;
    else if (!strcmp(argv[1], "twt"))    c.tweeter_gain_db = v;
    else if (!strcmp(argv[1], "master")) c.master_gain_db  = v;
    else { printf("unknown target\n"); return 1; }

    dsp_set_config(&c);
    printf("%s trim %+.1f dB\n", argv[1], v);
    return 0;
}

static int cmd_delay(int argc, char **argv)
{
    if (argc < 2) { printf("usage: delay <samples 0-%d>\n", DSP_MAX_DELAY_SAMPLES - 1); return 1; }
    dsp_config_t c; dsp_get_config(&c);
    c.tweeter_delay = (uint16_t)atoi(argv[1]);
    dsp_set_config(&c);
    printf("tweeter delay %u samples\n", c.tweeter_delay);
    return 0;
}

static int cmd_invert(int argc, char **argv)
{
    if (argc < 2) { printf("usage: invert <0|1>\n"); return 1; }
    dsp_config_t c; dsp_get_config(&c);
    c.tweeter_invert = atoi(argv[1]) != 0;
    dsp_set_config(&c);
    printf("tweeter polarity %s\n", c.tweeter_invert ? "inverted" : "normal");
    return 0;
}

static int cmd_save(int argc, char **argv)
{
    dsp_config_t c; dsp_get_config(&c);
    printf(settings_save(&c) == ESP_OK ? "saved\n" : "save failed\n");
    return 0;
}

static int cmd_load(int argc, char **argv)
{
    dsp_config_t c;
    settings_load(&c);
    dsp_set_config(&c);
    printf("loaded\n");
    return 0;
}

static int cmd_defaults(int argc, char **argv)
{
    dsp_config_t c;
    dsp_default_config(&c);
    dsp_set_config(&c);
    settings_erase();
    printf("reset to defaults\n");
    return 0;
}

static int cmd_chime(int argc, char **argv)
{
    chime_id_t id = CHIME_STARTUP;
    if (argc > 1) {
        if      (!strcmp(argv[1], "connect"))    id = CHIME_CONNECT;
        else if (!strcmp(argv[1], "disconnect")) id = CHIME_DISCONNECT;
        else if ( strcmp(argv[1], "startup")) {
            printf("usage: chime [startup|connect|disconnect]\n");
            return 1;
        }
    }
    audio_out_play_chime(id);
    printf("playing\n");
    return 0;
}

static int cmd_led(int argc, char **argv)
{
    if (argc < 2) {
        printf("LED on GPIO%d (%s)\n", led_viz_get_pin(),
               led_viz_is_active_low() ? "active low" : "active high");
        printf("usage: led <gpio> [al] | led test <0-100> | led auto\n");
        return 0;
    }

    if (!strcmp(argv[1], "auto")) {
        led_viz_override(-1.0f);
        printf("visualiser resumed\n");
        return 0;
    }

    if (!strcmp(argv[1], "test")) {
        float pct = argc > 2 ? strtof(argv[2], NULL) : 100.0f;
        led_viz_override(pct / 100.0f);
        printf("holding LED at %.0f%% — 'led auto' to resume\n", pct);
        return 0;
    }

    int gpio = atoi(argv[1]);
    bool al = (argc > 2 && !strcmp(argv[2], "al"));
    if (led_viz_set_pin((gpio_num_t)gpio, al) != ESP_OK) {
        printf("GPIO%d is not a valid output\n", gpio);
        return 1;
    }
    printf("LED moved to GPIO%d (%s)\n", gpio, al ? "active low" : "active high");
    return 0;
}

static int cmd_mute(int argc, char **argv)   { audio_out_mute();   printf("muted\n");   return 0; }
static int cmd_unmute(int argc, char **argv) { audio_out_unmute(); printf("un-muted\n"); return 0; }

static void reg(const char *cmd, const char *help, esp_console_cmd_func_t fn)
{
    const esp_console_cmd_t c = { .command = cmd, .help = help, .func = fn };
    ESP_ERROR_CHECK(esp_console_cmd_register(&c));
}

void shell_start(void)
{
    esp_console_repl_t *repl = NULL;
    esp_console_repl_config_t rc = ESP_CONSOLE_REPL_CONFIG_DEFAULT();
    rc.prompt = "sf01>";
    rc.max_cmdline_length = 128;

    esp_console_dev_uart_config_t uc = ESP_CONSOLE_DEV_UART_CONFIG_DEFAULT();
    if (esp_console_new_repl_uart(&uc, &rc, &repl) != ESP_OK) return;

    /*
     * Force the dumb-terminal path.
     *
     * esp_console probes for VT100 support by emitting ESC[5n and treating
     * any 4 bytes that arrive within 500 ms as the reply. A script that
     * starts talking early — or a user who types during boot — trips that
     * test, after which linenoise drives escape sequences and cursor-position
     * queries at a client that cannot answer, and the console floods the UART.
     * Nothing here needs line editing or history, so take the predictable
     * path that behaves identically for idf.py monitor, screen and scripts.
     */
    linenoiseSetDumbMode(1);

    esp_console_register_help_command();
    reg("status",   "Show crossover, EQ and output state",              cmd_status);
    reg("vol",      "vol <0-127>",                                      cmd_vol);
    reg("xo",       "xo <hz>  — set crossover frequency",               cmd_xo);
    reg("eq",       "eq <band> <off|peak|lowshelf|highshelf> <f> <Q> <dB>", cmd_eq);
    reg("gain",     "gain <wfr|twt|master> <dB>",                       cmd_gain);
    reg("delay",    "delay <samples>  — time-align the tweeter",        cmd_delay);
    reg("invert",   "invert <0|1>  — tweeter polarity",                 cmd_invert);
    reg("save",     "Persist the current tuning to NVS",                cmd_save);
    reg("load",     "Reload the tuning saved in NVS",                   cmd_load);
    reg("defaults", "Restore built-in defaults and erase NVS",          cmd_defaults);
    reg("chime",    "chime [startup|connect|disconnect]  — play a tone",  cmd_chime);
    reg("led",      "led <gpio> [al] | led test <0-100> | led auto",     cmd_led);
    reg("mute",     "Mute amplifiers and DACs",                         cmd_mute);
    reg("unmute",   "Un-mute, DAC first then amplifiers",               cmd_unmute);

    ESP_ERROR_CHECK(esp_console_start_repl(repl));
}
