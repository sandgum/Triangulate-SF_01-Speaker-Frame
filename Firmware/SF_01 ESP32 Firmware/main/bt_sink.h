/* bt_sink.h — Bluetooth Classic A2DP sink. */
#pragma once

#include "esp_err.h"
#include <stdbool.h>

esp_err_t bt_sink_start(const char *device_name);
bool      bt_sink_is_connected(void);
