
#include "network_send.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include <string.h>
#include <math.h>
#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include "nvs_flash.h"
#include "config.h"
#include "sdkconfig.h"
#include "audio_element.h"
#include "audio_pipeline.h"
#include "audio_event_iface.h"
#include "audio_common.h"
#include "http_stream.h"
#include "i2s_stream.h"
#include "mp3_decoder.h"

#include "esp_peripherals.h"
#include "periph_wifi.h"
#include "board.h"
#include "esp_http_client.h"
#include "esp_timer.h"



void send_wav_via_http(int32_t *pcm32, size_t num_samples) {
    // Размер данных в 16-битном представлении
    size_t pcm16_bytes = num_samples * sizeof(int16_t);
    size_t wav_size = 44 + pcm16_bytes;

    uint8_t *wav_buf = heap_caps_malloc(wav_size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (!wav_buf) {
        ESP_LOGE("HTTP", "No memory for WAV buffer");
        return;
    }

    // --- Заполняем WAV-заголовок (44 байта) ---
    // RIFF header
    memcpy(wav_buf, "RIFF", 4);
    uint32_t file_size = wav_size - 8;
    memcpy(wav_buf + 4, &file_size, 4);
    memcpy(wav_buf + 8, "WAVE", 4);

    // fmt subchunk
    memcpy(wav_buf + 12, "fmt ", 4);
    uint32_t fmt_size = 16;
    memcpy(wav_buf + 16, &fmt_size, 4);
    uint16_t audio_format = 1;      // PCM
    memcpy(wav_buf + 20, &audio_format, 2);
    uint16_t num_channels = 1;      // моно
    memcpy(wav_buf + 22, &num_channels, 2);
    uint32_t sample_rate = MIC_SAMPLE_RATE;
    memcpy(wav_buf + 24, &sample_rate, 4);
    uint32_t byte_rate = sample_rate * num_channels * sizeof(int16_t);
    memcpy(wav_buf + 28, &byte_rate, 4);
    uint16_t block_align = num_channels * sizeof(int16_t);
    memcpy(wav_buf + 32, &block_align, 2);
    uint16_t bits_per_sample = 16;
    memcpy(wav_buf + 34, &bits_per_sample, 2);

    // data subchunk
    memcpy(wav_buf + 36, "data", 4);
    uint32_t data_size = pcm16_bytes;
    memcpy(wav_buf + 40, &data_size, 4);

    // --- Конвертируем 32-битные сэмплы в 16-битные (сдвиг вправо на 16 бит) ---
    int16_t *pcm16 = (int16_t*)(wav_buf + 44);
    for (size_t i = 0; i < num_samples; i++) {
        pcm16[i] = (int16_t)(pcm32[i] >> 16);
    }

    // --- Отправка HTTP POST ---
    esp_http_client_config_t config = {
        .url = "http://192.168.1.52:3001/api/upload", // ваш endpoint
        .method = HTTP_METHOD_POST,
        .timeout_ms = 5000,
    };
    esp_http_client_handle_t client = esp_http_client_init(&config);
    esp_http_client_set_post_field(client, (char*)wav_buf, wav_size);
    esp_http_client_set_header(client, "Content-Type", "audio/wav");

    esp_err_t err = esp_http_client_perform(client);
    if (err == ESP_OK) {
        ESP_LOGI("HTTP", "WAV sent, status=%d", esp_http_client_get_status_code(client));
    } else {
        ESP_LOGE("HTTP", "Send failed: %s", esp_err_to_name(err));
    }
    esp_http_client_cleanup(client);
    free(wav_buf);
}


void send_raw_audio(int32_t *data, size_t samples) {
    size_t bytes = samples * sizeof(int32_t);

    esp_http_client_config_t config = {
        .url = "http://192.168.1.52:3001/api/upload_raw", // другой endpoint
        .method = HTTP_METHOD_POST,
        .timeout_ms = 5000,
    };
    esp_http_client_handle_t client = esp_http_client_init(&config);
    esp_http_client_set_post_field(client, (char*)data, bytes);
    // Можно указать Content-Type как application/octet-stream
    esp_http_client_set_header(client, "Content-Type", "application/octet-stream");

    esp_err_t err = esp_http_client_perform(client);
    if (err == ESP_OK) {
        ESP_LOGI("HTTP", "Raw data sent, status=%d", esp_http_client_get_status_code(client));
    } else {
        ESP_LOGE("HTTP", "Send failed: %s", esp_err_to_name(err));
    }
    esp_http_client_cleanup(client);
}

esp_err_t send_audio_get_response(int32_t *pcm32, size_t samples, 
                                  uint8_t **out_buf, size_t *out_size) {
    // 1. Формируем WAV как раньше
    size_t pcm16_bytes = samples * sizeof(int16_t);
    size_t wav_size = 44 + pcm16_bytes;
    uint8_t *wav_buf = heap_caps_malloc(wav_size, MALLOC_CAP_SPIRAM);
    if (!wav_buf) return ESP_ERR_NO_MEM;
    // ... заполняем WAV заголовок и данные (как в send_wav_via_http)
    
    // 2. Настраиваем HTTP POST с ожиданием ответа
    esp_http_client_config_t config = {
        .url = "http://192.168.1.52:3001/api/command",
        .method = HTTP_METHOD_POST,
        .timeout_ms = 30000,  // ждём до 10 секунд
        .buffer_size = 4096,  // достаточно для TTS ответа
    };
    
   esp_http_client_handle_t client = esp_http_client_init(&config);
    if (!client) {
        free(wav_buf);
        return ESP_FAIL;
    }

    esp_http_client_set_header(client, "Content-Type", "audio/wav");

    // Открываем соединение и отправляем заголовки с указанием размера тела
    esp_err_t err = esp_http_client_open(client, wav_size);
    if (err != ESP_OK) {
        ESP_LOGE("HTTP", "esp_http_client_open failed: %s", esp_err_to_name(err));
        esp_http_client_cleanup(client);
        free(wav_buf);
        return err;
    }

    // Отправляем тело (WAV данные)
    int written = esp_http_client_write(client, (char*)wav_buf, wav_size);
    if (written != wav_size) {
        ESP_LOGE("HTTP", "Written %d, expected %d", written, wav_size);
        esp_http_client_close(client);
        esp_http_client_cleanup(client);
        free(wav_buf);
        return ESP_FAIL;
    }

    // Получаем заголовки ответа
    int64_t content_length = esp_http_client_fetch_headers(client);
    int status = esp_http_client_get_status_code(client);
    ESP_LOGI("HTTP", "Response status: %d, content-length: %lld", status, content_length);

    if (status != 200 || content_length <= 0) {
        ESP_LOGE("HTTP", "Invalid response");
        esp_http_client_close(client);
        esp_http_client_cleanup(client);
        free(wav_buf);
        return ESP_FAIL;
    }

    // Выделяем буфер под ответ
    *out_buf = malloc(content_length);
    if (!*out_buf) {
        ESP_LOGE("HTTP", "Failed to allocate %lld bytes", content_length);
        esp_http_client_close(client);
        esp_http_client_cleanup(client);
        free(wav_buf);
        return ESP_ERR_NO_MEM;
    }

    // Читаем ответ по частям
    int total_read = 0;
    while (total_read < content_length) {
        int read = esp_http_client_read(client, (char*)(*out_buf) + total_read,
                                        content_length - total_read);
        if (read < 0) {
            ESP_LOGE("HTTP", "Read error: %d", read);
            free(*out_buf);
            *out_buf = NULL;
            esp_http_client_close(client);
            esp_http_client_cleanup(client);
            free(wav_buf);
            return ESP_FAIL;
        }
        if (read == 0) {
            ESP_LOGW("HTTP", "Connection closed prematurely, read %d/%lld", total_read, content_length);
            break;
        }
        total_read += read;
        ESP_LOGI("HTTP", "Read chunk %d, total %d", read, total_read);
    }

    esp_http_client_close(client);
    esp_http_client_cleanup(client);
    free(wav_buf);

    if (total_read != content_length) {
        ESP_LOGE("HTTP", "Incomplete read: %d/%lld", total_read, content_length);
        free(*out_buf);
        *out_buf = NULL;
        return ESP_FAIL;
    }

    *out_size = total_read;
    ESP_LOGI("HTTP", "Successfully read %d bytes", total_read);
    return ESP_OK;
}