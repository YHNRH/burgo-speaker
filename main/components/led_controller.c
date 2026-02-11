#include "led_controller.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

led_strip_handle_t led_controller_init(void) {
    led_strip_config_t strip_config = {
        .strip_gpio_num = LED_GPIO,
        .max_leds = LED_COUNT,
        .led_model = LED_MODEL_WS2812,
    };

    led_strip_rmt_config_t rmt_config = {
        .resolution_hz = 10 * 1000 * 1000, // 10 MHz
    };

    led_strip_handle_t led_strip;
    ESP_ERROR_CHECK(led_strip_new_rmt_device(&strip_config, &rmt_config, &led_strip));
    ESP_LOGI(LED_TAG, "LED strip initialized on GPIO %d", LED_GPIO);
    return led_strip;
}

void led_controller_set_brightness(led_strip_handle_t led_strip, uint8_t brightness_val) {
    for (int i = 0; i < LED_COUNT; i++) {
        ESP_ERROR_CHECK(led_strip_set_pixel(led_strip, i, 
                      brightness_val, brightness_val, brightness_val));
    }
    ESP_ERROR_CHECK(led_strip_refresh(led_strip));
}

void led_controller_clear(led_strip_handle_t led_strip) {
    ESP_ERROR_CHECK(led_strip_clear(led_strip));
}

void led_controller_test_pattern(led_strip_handle_t led_strip, bool *pattern_active) {
    if (!(*pattern_active)) return;
    
    // 1. Красный бегущий огонь
    for (int i = 0; i < LED_COUNT; i++) {
        ESP_ERROR_CHECK(led_strip_set_pixel(led_strip, i, 255, 0, 0));
        ESP_ERROR_CHECK(led_strip_refresh(led_strip));
        vTaskDelay(pdMS_TO_TICKS(200));
        ESP_ERROR_CHECK(led_strip_set_pixel(led_strip, i, 0, 0, 0));
        
        if (!(*pattern_active)) break;
    }

    if (!(*pattern_active)) return;

    // 2. Зелёное заполнение
    for (int i = 0; i < LED_COUNT; i++) {
        ESP_ERROR_CHECK(led_strip_set_pixel(led_strip, i, 0, 255, 0));
    }
    ESP_ERROR_CHECK(led_strip_refresh(led_strip));
    vTaskDelay(pdMS_TO_TICKS(1000));

    if (!(*pattern_active)) return;

    // 3. Синяя "комета"
    for (int pos = 0; pos < LED_COUNT * 2; pos++) {
        for (int i = 0; i < LED_COUNT; i++) {
            int brightness_val = 255 - abs(i - pos) * 50;
            if (brightness_val < 0) brightness_val = 0;
            ESP_ERROR_CHECK(led_strip_set_pixel(led_strip, i, 0, 0, brightness_val));
        }
        ESP_ERROR_CHECK(led_strip_refresh(led_strip));
        vTaskDelay(pdMS_TO_TICKS(100));
        
        if (!(*pattern_active)) break;
    }
}