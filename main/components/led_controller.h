#ifndef __LED_CONTROLLER_H__
#define __LED_CONTROLLER_H__

#include "led_strip.h"
#include "config.h"

// Инициализация светодиодов
led_strip_handle_t led_controller_init(void);

// Установка яркости для всех светодиодов
void led_controller_set_brightness(led_strip_handle_t led_strip, uint8_t brightness);

// Очистка светодиодов
void led_controller_clear(led_strip_handle_t led_strip);

// Тестовый паттерн
void led_controller_test_pattern(led_strip_handle_t led_strip, bool *pattern_active);

#endif // __LED_CONTROLLER_H__