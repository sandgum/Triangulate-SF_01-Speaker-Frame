/*
 * led_viz.h — bass visualiser on the devboard's onboard LED.
 *
 * Drives the LED with PWM (not on/off) so it actually pulses with the music
 * rather than blinking. The level comes from dsp_bass_level(), a ~120 Hz
 * band tapped off the woofer branch after its gain.
 *
 * The pin defaults to GPIO2, which is where essentially every generic ESP32
 * devkit puts its user LED. If yours is elsewhere, the `led` shell command
 * retargets it at runtime so you can find it without a rebuild.
 *
 * GPIO2 is a strapping pin, so this is initialised well after boot — by then
 * it is an ordinary output and driving it is exactly what the devkits do.
 */
#pragma once

#include <stdbool.h>
#include "esp_err.h"
#include "driver/gpio.h"

esp_err_t led_viz_start(void);

/* Retarget the LED at runtime (shell `led <gpio> [activelow]`). */
esp_err_t led_viz_set_pin(gpio_num_t gpio, bool active_low);

/* Hold a fixed brightness 0..1 for pin hunting; < 0 resumes the visualiser. */
void      led_viz_override(float level);

gpio_num_t led_viz_get_pin(void);
bool       led_viz_is_active_low(void);
