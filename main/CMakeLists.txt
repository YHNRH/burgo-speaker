
idf_component_register(SRCS main.c components/button_controller.c components/led_controller.c components/audio_output.c
                       REQUIRES ${requires} audio_stream esp_wifi esp_peripherals)

