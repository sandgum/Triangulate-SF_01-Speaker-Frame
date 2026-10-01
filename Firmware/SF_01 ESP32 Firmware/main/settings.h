/* settings.h — persist the DSP tuning in NVS so it survives a power cycle. */
#pragma once

#include "dsp.h"
#include "esp_err.h"

/* Loads the saved tuning into *cfg, or the defaults if nothing is stored
 * or the stored blob is from an incompatible firmware version. */
void      settings_load(dsp_config_t *cfg);
esp_err_t settings_save(const dsp_config_t *cfg);
esp_err_t settings_erase(void);
