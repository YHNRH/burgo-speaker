#include "mic_task.h"


#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_timer.h"
#include <math.h>
#include "../components/config.h"
#include "../components/network_send.h"
#include "../components/mic_i2s.h"
#include "../components/led_controller.h"
#include "esp_log.h"

#include <string.h>

// Структура состояния задачи
typedef enum {
    MIC_STATE_IDLE,
    MIC_STATE_RECORDING
} mic_state_t;

#include "../wake_word.h"

static const char *TAG = "MIC_TASK";
// Глобальные переменные состояния
static mic_state_t state = MIC_STATE_IDLE;
static int32_t *record_buf = NULL;
static size_t record_pos = 0;
static int chunk_size = 0; // получим после инициализации

static void mic_task(void *arg) {
    // Выделяем буфер для чтения (размером chunk_size)
    int32_t *sample_buf = heap_caps_malloc(chunk_size * sizeof(int32_t), MALLOC_CAP_DMA);
    // Буфер для преобразованных 16-битных сэмплов
    int16_t *sample16 = heap_caps_malloc(chunk_size * sizeof(int16_t), MALLOC_CAP_INTERNAL);

    // Буфер для записи (3 секунды) – как раньше
    record_buf = heap_caps_malloc(RECORD_SAMPLES * sizeof(int32_t), MALLOC_CAP_SPIRAM);

    if (!sample_buf || !sample16 || !record_buf) {
        ESP_LOGE(TAG, "Memory allocation failed");
        vTaskDelete(NULL);
        return;
    }
    
    while (1) {
        // Читаем ровно chunk_size сэмплов
        esp_err_t ret = mic_i2s_read(sample_buf, chunk_size, portMAX_DELAY);
        if (ret != ESP_OK) continue;

        // Преобразуем в 16 бит (сдвиг на 16)
        for (int i = 0; i < chunk_size; i++) {
            sample16[i] = (int16_t)(sample_buf[i] >> 16);
        }

        switch (state) {
            case MIC_STATE_IDLE: {
                // Передаём в детектор
                if (wake_word_process(sample16, chunk_size)) {
                    ESP_LOGI(TAG, "Wake word detected!");
                    // Переходим в режим записи
                    state = MIC_STATE_RECORDING;
                    record_pos = 0;
                    // Копируем текущий блок в начало записи (чтобы не потерять начало фразы)
                    memcpy(record_buf, sample_buf, chunk_size * sizeof(int32_t));
                    record_pos += chunk_size;
                }
                break;
            }

            case MIC_STATE_RECORDING: {
                // Копируем в буфер записи
                size_t remaining = RECORD_SAMPLES - record_pos;
                size_t to_copy = (chunk_size < remaining) ? chunk_size : remaining;
                memcpy(record_buf + record_pos, sample_buf, to_copy * sizeof(int32_t));
                record_pos += to_copy;

                led_controller_set_state(LED_STATE_WAKE_WORD_DETECTED); 
                if (record_pos >= RECORD_SAMPLES) {
                    ESP_LOGI(TAG, "Recording finished, sending...");
                    send_wav_via_http(record_buf, RECORD_SAMPLES);
                    state = MIC_STATE_IDLE;
                    led_controller_set_state(LED_STATE_IDLE); 
                }
                break;
            }
        }
    }
}

void audio_processing_start(void) {
    // В audio_processing_start() или перед созданием задачи
        ESP_ERROR_CHECK(wake_word_init("sophia"));  // или "multinet", "mycroft"
    chunk_size = wake_word_get_chunk_size();

    // Создаём задачу
    xTaskCreatePinnedToCore(mic_task, "mic_task", 8192, NULL, 5, NULL, 0);
}