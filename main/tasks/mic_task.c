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
// В начале файла
static bool use_afe = false; // сначала false

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
    int32_t *raw_buf = heap_caps_malloc(chunk_size * sizeof(int32_t), MALLOC_CAP_DMA);
    int16_t *sample16 = heap_caps_malloc(chunk_size * sizeof(int16_t), MALLOC_CAP_DMA);

    if (!raw_buf || !sample16) {
        ESP_LOGE(TAG, "Failed to allocate buffers");
        vTaskDelete(NULL);
        return;
    }

    // Локальные переменные состояния (static, чтобы сохранялись между вызовами)
    static mic_state_t state = MIC_STATE_IDLE;
    static size_t record_pos = 0;

    while (1) {
        // Читаем сырые данные с I2S
        esp_err_t ret = mic_i2s_read(raw_buf, chunk_size, portMAX_DELAY);
        if (ret != ESP_OK) continue;

        // Преобразуем в int16_t (сдвиг на 16 бит)
        for (int i = 0; i < chunk_size; i++) {
            sample16[i] = (int16_t)(raw_buf[i] >> 16);
        }

        // 3. Если используем AFE
        if (use_afe) {
            int16_t *processed = NULL;
            size_t proc_samples = 0;
            if (afe_process(sample16, chunk_size, &processed, &proc_samples)) {
                if (processed && proc_samples > 0) {
                    // 3a. Проверяем детекцию wake word
                    if (wake_word_process(processed, proc_samples)) {
                        ESP_LOGI(TAG, "Wake word detected!");
                        led_controller_set_state(LED_STATE_WAKE_WORD_DETECTED);
                        state = MIC_STATE_RECORDING;
                        record_pos = 0;
                        // Копируем текущий блок в буфер записи
                        for (size_t i = 0; i < proc_samples && record_pos < RECORD_SAMPLES; i++) {
                            record_buf[record_pos++] = (int32_t)processed[i] << 16;
                        }
                    }
                    // 3b. Если уже в записи, продолжаем копировать
                    else if (state == MIC_STATE_RECORDING) {
                        for (size_t i = 0; i < proc_samples && record_pos < RECORD_SAMPLES; i++) {
                            record_buf[record_pos++] = (int32_t)processed[i] << 16;
                        }
                        if (record_pos >= RECORD_SAMPLES) {
                            ESP_LOGI(TAG, "Recording finished, sending...");
                            send_wav_via_http(record_buf, RECORD_SAMPLES);
                            state = MIC_STATE_IDLE;
                            led_controller_set_state(LED_STATE_IDLE);
                        }
                    }
                }
            }
        } else {
            // Старая ветка (без AFE)
            if (wake_word_process(sample16, chunk_size)) {
                ESP_LOGI(TAG, "Wake word detected!");
                led_controller_set_state(LED_STATE_WAKE_WORD_DETECTED);
                state = MIC_STATE_RECORDING;
                record_pos = 0;
                memcpy(record_buf, raw_buf, chunk_size * sizeof(int32_t));
                record_pos += chunk_size;
            } else if (state == MIC_STATE_RECORDING) {
                size_t remaining = RECORD_SAMPLES - record_pos;
                size_t to_copy = (chunk_size < remaining) ? chunk_size : remaining;
                memcpy(record_buf + record_pos, raw_buf, to_copy * sizeof(int32_t));
                record_pos += to_copy;
                if (record_pos >= RECORD_SAMPLES) {
                    ESP_LOGI(TAG, "Recording finished, sending...");
                    send_wav_via_http(record_buf, RECORD_SAMPLES);
                    state = MIC_STATE_IDLE;
                    led_controller_set_state(LED_STATE_IDLE); 
                }
            }
        }
    }

    free(raw_buf);
    free(sample16);
    vTaskDelete(NULL);
}

void audio_processing_start(void) {
    // В audio_processing_start() или перед созданием задачи
        ESP_ERROR_CHECK(wake_word_init("sophia"));  // или "multinet", "mycroft"
    chunk_size = wake_word_get_chunk_size();

    // Теперь AFE (пока для теста, но не используем)
    esp_err_t err = afe_init("sophia");
    if (err != ESP_OK)
    {
        ESP_LOGW(TAG, "AFE init failed, will use old wake word");
    }
    else
    {
        ESP_LOGI(TAG, "AFE ready, chunk size: %d", afe_chunk_size);

        if (afe_handle != NULL) {
    use_afe = true;
    chunk_size = afe_chunk_size;  // используем размер от AFE
    ESP_LOGI(TAG, "Using AFE, chunk size: %d", chunk_size);
} else {
    chunk_size = wake_word_get_chunk_size();
    ESP_LOGI(TAG, "Using old wake word, chunk size: %d", chunk_size);
}
    }

    record_buf = heap_caps_malloc(RECORD_SAMPLES * sizeof(int32_t), MALLOC_CAP_SPIRAM);

    // Создаём задачу
    xTaskCreatePinnedToCore(mic_task, "mic_task", 8192, NULL, 5, NULL, 0);
}