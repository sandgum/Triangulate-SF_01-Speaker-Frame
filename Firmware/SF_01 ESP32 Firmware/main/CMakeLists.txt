idf_component_register(
    SRCS        "main.c"
                "audio_out.c"
                "dsp.c"
                "bt_sink.c"
                "settings.c"
                "shell.c"
                "ble_ctl.c"
                "led_viz.c"
                "chime.c"
                "fw_update.c"
    INCLUDE_DIRS "."
    REQUIRES    bt
                nvs_flash
                console
                esp_driver_i2s
                esp_driver_gpio
                esp_driver_ledc
                esp_ringbuf
                esp_timer
                app_update
                esp_app_format
)
