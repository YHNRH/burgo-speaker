#include "mic_task.h"


#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_timer.h"
#include <math.h>
#include "../components/config.h"
#include "../components/network_send.h"
#include "../components/mic_i2s.h"
#include "esp_log.h"

#include <string.h>

// Структура состояния задачи
typedef enum {
    MIC_STATE_IDLE,
    MIC_STATE_RECORDING
} mic_state_t;

void mic_task(void *arg) {
    // Буфер для чтения пачек (512 сэмплов)
    int32_t *sample_buf = heap_caps_malloc(512 * sizeof(int32_t), MALLOC_CAP_DMA);
    if (!sample_buf) {
        ESP_LOGE("MIC", "Failed to allocate sample buffer");
        vTaskDelete(NULL);
        return;
    }
    
    // Буфер для записи (3 секунды)
    int32_t *record_buf = heap_caps_malloc(RECORD_SAMPLES * sizeof(int32_t), MALLOC_CAP_SPIRAM);
    if (!record_buf) {
        ESP_LOGE("MIC", "Failed to allocate record buffer");
        free(sample_buf);
        vTaskDelete(NULL);
        return;
    }
    
    mic_state_t state = MIC_STATE_IDLE;
    size_t record_pos = 0;
    int64_t record_start_time = 0;
    
    while (1) {
        // Читаем пачку
        esp_err_t ret = mic_i2s_read(sample_buf, 512, portMAX_DELAY);
        if (ret != ESP_OK) {
            continue;
        }

        // После успешного mic_i2s_read
        ESP_LOGI("MIC_RAW", "samples[0]=%ld, [1]=%ld, [2]=%ld, [3]=%ld",
         sample_buf[0], sample_buf[1], sample_buf[2], sample_buf[3]);
        
        // Вычисляем RMS для этой пачки
        int64_t sum_sq = 0;
        for (int i = 0; i < 512; i++) {
            sum_sq += (int64_t)sample_buf[i] * sample_buf[i];
        }
        int32_t rms = (int32_t)sqrt(sum_sq / 512);
        
        // Логируем RMS для отладки
        //ESP_LOGI("MIC", "RMS=%ld", rms);
        
        // Автомат состояний
        switch (state) {
            case MIC_STATE_IDLE:
                if (rms > RMS_TRIGGER_THRESH) {
                    state = MIC_STATE_RECORDING;
                    record_pos = 0;
                    record_start_time = esp_timer_get_time();
                    ESP_LOGI("MIC", "Start recording...");
                }
                break;
                
            case MIC_STATE_RECORDING: {
                // Сколько сэмплов можно скопировать без переполнения буфера
                size_t remaining = RECORD_SAMPLES - record_pos;
                size_t to_copy = (512 < remaining) ? 512 : remaining;
                memcpy(record_buf + record_pos, sample_buf, to_copy * sizeof(int32_t));
                record_pos += to_copy;
                
                // Если набрали достаточно
                if (record_pos >= RECORD_SAMPLES) {
                    ESP_LOGI("MIC", "Recording finished, sending...");
                    // Конвертируем в int16_t и отправляем
                    // Пока у нас record_buf заполнен int32_t, но send_wav_via_http ожидает int16_t*
                    // Поэтому передадим int32_t и преобразуем внутри функции (или адаптируем)
                    send_wav_via_http(record_buf, RECORD_SAMPLES);
                    //send_raw_audio(record_buf, RECORD_SAMPLES);
                    state = MIC_STATE_IDLE;
                }
                break;
            }
        }
    }
    
    free(sample_buf);
    free(record_buf);
    vTaskDelete(NULL);
}
