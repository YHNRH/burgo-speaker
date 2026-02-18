#include "led_controller.h"
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "led_strip.h"
#include "math.h"
#include "config.h"

#define RENDER_PERIOD_MS   20                      // 50 FPS

static const char *TAG = LED_TAG;

// Дескриптор ленты
static led_strip_handle_t g_led_strip = NULL;

// Текущее состояние колонки
static led_state_t g_current_state = LED_STATE_IDLE;

// Для временных событий
static led_state_t g_previous_state = LED_STATE_IDLE;
static esp_timer_handle_t g_event_timer = NULL;
static SemaphoreHandle_t g_state_mutex = NULL;

// Функции рендера эффектов (возвращают цвета для всех диодов)
typedef void (*render_func_t)(uint32_t now_ms, uint8_t colors[LED_COUNT][3]);

// Описатель эффекта
typedef struct {
    render_func_t render;
    void *params;  // параметры, специфичные для эффекта
} effect_t;

// Параметры для разных типов эффектов
typedef struct {
    uint8_t r, g, b;
    uint8_t min_brightness;
    uint8_t max_brightness;
    uint16_t period_ms;
} breathe_params_t;

typedef struct {
    uint8_t r, g, b;
    uint16_t on_ms;
    uint16_t off_ms;
} blink_params_t;

typedef struct {
    uint8_t colors[LED_COUNT][3];
    uint16_t step_ms;   // интервал сдвига (0 – статика)
} gradient_params_t;

typedef struct {
    uint8_t r, g, b;
    uint16_t period_ms;  // время полного прохода волны
} wave_params_t;

// Объявления функций рендера
static void render_static_color(uint32_t now_ms, uint8_t colors[LED_COUNT][3]);
static void render_breathe(uint32_t now_ms, uint8_t colors[LED_COUNT][3]);
static void render_blink(uint32_t now_ms, uint8_t colors[LED_COUNT][3]);
static void render_gradient(uint32_t now_ms, uint8_t colors[LED_COUNT][3]);
static void render_wave(uint32_t now_ms, uint8_t colors[LED_COUNT][3]);

// Таблица соответствия состояний и эффектов
static struct {
    render_func_t render;
    void *params;
} s_state_effects[LED_STATE_COUNT] = {
    [LED_STATE_IDLE] = {
        .render = render_breathe,
        .params = &(breathe_params_t){ .r = 0, .g = 0, .b = 255, .min_brightness = 20, .max_brightness = 200, .period_ms = 2000 }
    },
    [LED_STATE_WAKE_WORD_DETECTED] = {
        .render = render_blink,
        .params = &(blink_params_t){ .r = 0, .g = 255, .b = 0, .on_ms = 100, .off_ms = 100 }
    },
    [LED_STATE_LISTENING] = {
        .render = render_static_color,
        .params = &(breathe_params_t){ .r = 255, .g = 255, .b = 255 } // яркость полная
    },
    [LED_STATE_PLAYING] = {
        .render = render_gradient,
        .params = &(gradient_params_t){
            .colors = { {255,0,0}, {0,255,0}, {0,0,255} },
            .step_ms = 500
        }
    },
    [LED_STATE_VOLUME_CHANGE] = {
        .render = render_wave,
        .params = &(wave_params_t){ .r = 255, .g = 255, .b = 0, .period_ms = 500 }
    },
    [LED_STATE_ERROR] = {
        .render = render_blink,
        .params = &(blink_params_t){ .r = 255, .g = 0, .b = 0, .on_ms = 200, .off_ms = 200 }
    }
};

// ---------- Реализация эффектов ----------

static void render_static_color(uint32_t now_ms, uint8_t colors[LED_COUNT][3]) {
    breathe_params_t *p = s_state_effects[LED_STATE_LISTENING].params; // переиспользуем структуру
    for (int i = 0; i < LED_COUNT; i++) {
        colors[i][0] = p->r;
        colors[i][1] = p->g;
        colors[i][2] = p->b;
    }
    (void)now_ms;
}

static void render_breathe(uint32_t now_ms, uint8_t colors[LED_COUNT][3]) {
    breathe_params_t *p = s_state_effects[LED_STATE_IDLE].params;
    // Синусоида: brightness = min + (max-min)*(0.5 + 0.5*sin(phase))
    float phase = (float)(now_ms % p->period_ms) / p->period_ms * 2.0f * 3.14159f;
    float factor = 0.5f + 0.5f * sinf(phase);
    uint8_t brightness = p->min_brightness + (uint8_t)((p->max_brightness - p->min_brightness) * factor);
    for (int i = 0; i < LED_COUNT; i++) {
        colors[i][0] = (p->r * brightness) / 255;
        colors[i][1] = (p->g * brightness) / 255;
        colors[i][2] = (p->b * brightness) / 255;
    }
}

static void render_blink(uint32_t now_ms, uint8_t colors[LED_COUNT][3]) {
    // Используем параметры из состояния WAKE_WORD_DETECTED или ERROR
    // Определим, какой сейчас эффект (возьмём параметры из текущего состояния)
    blink_params_t *p = NULL;
    if (g_current_state == LED_STATE_WAKE_WORD_DETECTED)
        p = s_state_effects[LED_STATE_WAKE_WORD_DETECTED].params;
    else if (g_current_state == LED_STATE_ERROR)
        p = s_state_effects[LED_STATE_ERROR].params;
    else
        return; // не должно случиться

    uint32_t cycle = p->on_ms + p->off_ms;
    bool on = (now_ms % cycle) < p->on_ms;
    uint8_t r = on ? p->r : 0;
    uint8_t g = on ? p->g : 0;
    uint8_t b = on ? p->b : 0;
    for (int i = 0; i < LED_COUNT; i++) {
        colors[i][0] = r;
        colors[i][1] = g;
        colors[i][2] = b;
    }
}

static void render_gradient(uint32_t now_ms, uint8_t colors[LED_COUNT][3]) {
    gradient_params_t *p = s_state_effects[LED_STATE_PLAYING].params;
    // Если step_ms == 0 – статика
    if (p->step_ms == 0) {
        memcpy(colors, p->colors, sizeof(p->colors));
        return;
    }
    // Циклический сдвиг цветов каждые step_ms
    int shift = (now_ms / p->step_ms) % LED_COUNT;
    for (int i = 0; i < LED_COUNT; i++) {
        int src = (i + shift) % LED_COUNT;
        colors[i][0] = p->colors[src][0];
        colors[i][1] = p->colors[src][1];
        colors[i][2] = p->colors[src][2];
    }
}

static void render_wave(uint32_t now_ms, uint8_t colors[LED_COUNT][3]) {
    wave_params_t *p = s_state_effects[LED_STATE_VOLUME_CHANGE].params;
    // Позиция волны: от 0 до LED_COUNT-1, двигается слева направо за period_ms
    float pos = (float)(now_ms % p->period_ms) / p->period_ms * LED_COUNT;
    for (int i = 0; i < LED_COUNT; i++) {
        float dist = (float)i - pos;
        float intensity = expf(-dist * dist * 2.0f); // гауссиана
        if (intensity > 1.0f) intensity = 1.0f;
        colors[i][0] = (uint8_t)(p->r * intensity);
        colors[i][1] = (uint8_t)(p->g * intensity);
        colors[i][2] = (uint8_t)(p->b * intensity);
    }
}

// ---------- Задача рендера ----------
static void led_render_task(void *arg) {
    uint8_t colors[LED_COUNT][3] = {0};
    uint32_t last_render = 0;

    while (1) {
        uint32_t now = esp_timer_get_time() / 1000; // ms

        // Получаем текущее состояние под защитой мьютекса
        led_state_t state;
        xSemaphoreTake(g_state_mutex, portMAX_DELAY);
        state = g_current_state;
        xSemaphoreGive(g_state_mutex);

        // Выбираем функцию рендера для этого состояния
        render_func_t render = s_state_effects[state].render;
        if (render) {
            render(now, colors);
        } else {
            // Если нет рендера – гасим светодиоды
            memset(colors, 0, sizeof(colors));
        }

        // Применяем цвета к ленте
        for (int i = 0; i < LED_COUNT; i++) {
            esp_err_t err = led_strip_set_pixel(g_led_strip, i, colors[i][0], colors[i][1], colors[i][2]);
            if (err != ESP_OK) {
                ESP_LOGE(TAG, "Failed to set pixel %d: %s", i, esp_err_to_name(err));
            }
        }
        esp_err_t err = led_strip_refresh(g_led_strip);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Failed to refresh strip: %s", esp_err_to_name(err));
        }

        // Ждём следующий кадр
        vTaskDelay(pdMS_TO_TICKS(RENDER_PERIOD_MS));
    }
}

// ---------- Таймер для временных событий ----------
static void event_timer_callback(void *arg) {
    xSemaphoreTake(g_state_mutex, portMAX_DELAY);
    g_current_state = g_previous_state;
    xSemaphoreGive(g_state_mutex);
}

// ---------- Публичные функции ----------
esp_err_t led_controller_init(void) {
    // Инициализация мьютекса
    g_state_mutex = xSemaphoreCreateMutex();
    if (!g_state_mutex) {
        ESP_LOGE(TAG, "Failed to create mutex");
        return ESP_ERR_NO_MEM;
    }

    // Конфигурация ленты
    led_strip_config_t strip_config = {
        .strip_gpio_num = LED_GPIO,
        .max_leds = LED_COUNT,
        .led_model = LED_MODEL_WS2812,
    };
    led_strip_rmt_config_t rmt_config = {
        .resolution_hz = 10 * 1000 * 1000, // 10 MHz
    };
    esp_err_t err = led_strip_new_rmt_device(&strip_config, &rmt_config, &g_led_strip);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create LED strip: %s", esp_err_to_name(err));
        return err;
    }

    // Создаём таймер для временных событий
    esp_timer_create_args_t timer_args = {
        .callback = event_timer_callback,
        .arg = NULL,
        .name = "led_event_timer"
    };
    err = esp_timer_create(&timer_args, &g_event_timer);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create event timer: %s", esp_err_to_name(err));
        return err;
    }

    // Запускаем задачу рендера
    BaseType_t task_created = xTaskCreate(led_render_task, "led_render", 4096, NULL, 5, NULL);
    if (task_created != pdPASS) {
        ESP_LOGE(TAG, "Failed to create render task");
        return ESP_ERR_NO_MEM;
    }

    ESP_LOGI(TAG, "LED controller initialized");
    return ESP_OK;
}

esp_err_t led_controller_set_state(led_state_t new_state) {
    if (new_state >= LED_STATE_COUNT) return ESP_ERR_INVALID_ARG;

    xSemaphoreTake(g_state_mutex, portMAX_DELAY);
    // Если был активен временный таймер – останавливаем его
    if (esp_timer_is_active(g_event_timer)) {
        esp_timer_stop(g_event_timer);
    }
    g_current_state = new_state;
    xSemaphoreGive(g_state_mutex);

    return ESP_OK;
}

esp_err_t led_controller_trigger_event(led_state_t event_state, uint32_t duration_ms) {
    if (event_state >= LED_STATE_COUNT) return ESP_ERR_INVALID_ARG;

    xSemaphoreTake(g_state_mutex, portMAX_DELAY);
    g_previous_state = g_current_state;
    g_current_state = event_state;
    // Запускаем таймер для возврата
    esp_timer_start_once(g_event_timer, duration_ms * 1000);
    xSemaphoreGive(g_state_mutex);

    return ESP_OK;
}

esp_err_t led_controller_clear(void) {
    if (!g_led_strip) return ESP_ERR_INVALID_STATE;
    esp_err_t err = led_strip_clear(g_led_strip);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to clear strip: %s", esp_err_to_name(err));
    }
    return err;
}