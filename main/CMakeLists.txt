set(requires
    hardware_driver
    player
    )

idf_component_register(SRCS main.c components/button_controller.c components/led_controller.c components/audio_output.c
                       REQUIRES ${requires})

