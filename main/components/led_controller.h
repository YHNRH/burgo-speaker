#pragma once
#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"

// Перечисление возможных состояний колонки, на которые светодиоды реагируют
typedef enum {
    LED_STATE_IDLE,               // Ожидание голосовой команды
    LED_STATE_WAKE_WORD_DETECTED, // Пробуждение
    LED_STATE_LISTENING,          // Запись команды (после wake word)
    LED_STATE_PLAYING,            // Воспроизведение музыки
    LED_STATE_VOLUME_CHANGE,      // Изменение громкости (временный эффект)
    LED_STATE_ERROR,              // Ошибка (нет WiFi, проблемы с сервером)
    LED_STATE_COUNT
} led_state_t;

// Инициализация ленты (должна вызываться один раз при старте)
esp_err_t led_controller_init(void);

// Переключение состояния колонки – именно эту функцию будут вызывать другие модули
esp_err_t led_controller_set_state(led_state_t new_state);

// Если нужно кратковременно показать эффект, а затем вернуться к предыдущему состоянию
esp_err_t led_controller_trigger_event(led_state_t event_state, uint32_t duration_ms);

// Полное выключение всех светодиодов
esp_err_t led_controller_clear(void);