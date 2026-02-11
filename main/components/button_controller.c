#include "button_controller.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"

button_state_t button1_state = {TOUCH_SENSOR_1_GPIO, false, false, 0};
button_state_t button2_state = {TOUCH_SENSOR_2_GPIO, false, false, 0};

void button_controller_init(void) {
    // Конфигурация GPIO
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << TOUCH_SENSOR_1_GPIO) | (1ULL << TOUCH_SENSOR_2_GPIO),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_ENABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io_conf);

    // Инициализация начального состояния
    button1_state.last_state = gpio_get_level(button1_state.gpio);
    button2_state.last_state = gpio_get_level(button2_state.gpio);
    button1_state.pressed = false;
    button2_state.pressed = false;

    ESP_LOGI(BUTTON_TAG, "Buttons initialized");
}

bool button_controller_check_press(button_state_t *sensor) {
    // Чтение текущего состояния
    bool current_state = gpio_get_level(sensor->gpio);
    uint32_t now = pdTICKS_TO_MS(xTaskGetTickCount());
    
    // Антидребезг
    if (current_state != sensor->last_state) {
        if ((now - sensor->last_press_time) > DEBOUNCE_TIME_MS) {
            sensor->last_state = current_state;
            sensor->last_press_time = now;
            
            // Детектирование фронта нажатия
            if (current_state && !sensor->pressed) {
                sensor->pressed = true;
                return true;
            }
        }
    }
    
    // Сброс флага при отпускании
    if (!current_state && sensor->pressed) {
        sensor->pressed = false;
    }
    
    return false;
}