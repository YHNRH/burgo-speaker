
set(requires
    hardware_driver
    player
    )

idf_component_register(SRCS main.c components/button_controller.c components/led_controller.c components/audio_output.c components/mic_i2s.c components/network_send.c tasks/mic_task.c wake_word.c
                        INCLUDE_DIRS "."
                        PRIV_REQUIRES esp-sr
                       REQUIRES ${requires} esp-sr audio_stream esp_wifi esp_peripherals esp_http_client)

