/*
 * mic_i2s.c
 *
 * Работа с I2S-микрофоном через новый драйвер (ESP-IDF v5.0+)
 * Использует i2s_std.h для конфигурации в стандартном режиме.
 */

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_heap_caps.h"
#include "driver/i2s_std.h"      // Новый I2S-драйвер, стандартный режим
#include "driver/gpio.h"

#define MIC_I2S_PORT        I2S_NUM_1          // Используем второй I2S-порт
#define MIC_SAMPLE_RATE     16000              // 16 кГц для голоса
#define MIC_BITS_PER_SAMPLE I2S_DATA_BIT_WIDTH_32BIT // Микрофон выдаёт 32 бита
#define MIC_DMA_BUF_COUNT   8                  // Количество DMA-буферов
#define MIC_DMA_FRAME_LEN   256                // Количество фреймов в буфере

// ⚠️ УКАЖИТЕ СВОИ ПИНЫ! Рекомендуемые для S3: BCK=5, WS=45, DIN=6
#define MIC_BCK_IO          5
#define MIC_WS_IO           45
#define MIC_DATA_IN_IO      6

static const char *TAG = "MIC_I2S";
static i2s_chan_handle_t mic_rx_handle = NULL;   // Дескриптор канала

esp_err_t mic_i2s_init(void) {
    esp_err_t ret = ESP_OK;

    // 1. Конфигурация канала (общая для всех режимов)
    i2s_chan_config_t chan_cfg = {
        .id = MIC_I2S_PORT,
        .role = I2S_ROLE_MASTER,
        .dma_desc_num = MIC_DMA_BUF_COUNT,
        .dma_frame_num = MIC_DMA_FRAME_LEN,
        .auto_clear = true,               // Автоматически чистить буфер при переполнении
    };
    ret = i2s_new_channel(&chan_cfg, NULL, &mic_rx_handle); // RX-канал (второй аргумент - TX, он NULL)
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create I2S RX channel: %d", ret);
        return ret;
    }

    // 2. Конфигурация стандартного режима (std)
    i2s_std_config_t std_cfg = {
        .clk_cfg = {
            .sample_rate_hz = MIC_SAMPLE_RATE,
            .clk_src = I2S_CLK_SRC_DEFAULT,   // Источник тактов по умолчанию
            .mclk_multiple = I2S_MCLK_MULTIPLE_256, // Множитель MCLK (обычно 256)
        },
        .slot_cfg = {
            .data_bit_width = MIC_BITS_PER_SAMPLE,
            .slot_bit_width = I2S_SLOT_BIT_WIDTH_32BIT, // Ширина слота 32 бита
            .slot_mode = I2S_SLOT_MODE_MONO,     // Моно (один канал)
            .slot_mask = I2S_STD_SLOT_LEFT,       // Используем левый канал
            .ws_width = I2S_SLOT_BIT_WIDTH_32BIT, // Ширина WS (обычно равна ширине слота)
            .ws_pol = false,                       // Полярность WS: низкий уровень = начало кадра
            .bit_shift = true,                      // MSB first
            .left_align = true,                      // Выравнивание по левому краю
            .big_endian = false,
            .bit_order_lsb = false,
        },
        .gpio_cfg = {
            .bclk = MIC_BCK_IO,
            .ws = MIC_WS_IO,
            .dout = GPIO_NUM_NC,          // Не используется (только RX)
            .din = MIC_DATA_IN_IO,
            .invert_flags = {
                .bclk_inv = false,
                .ws_inv = false,
            //    .dout_inv = false,
           //     .din_inv = false,
            },
        },
    };

    ret = i2s_channel_init_std_mode(mic_rx_handle, &std_cfg);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to init I2S std mode: %d", ret);
        i2s_del_channel(mic_rx_handle);
        mic_rx_handle = NULL;
        return ret;
    }

    // 3. Включаем канал (начинаем приём)
    ret = i2s_channel_enable(mic_rx_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to enable I2S channel: %d", ret);
        i2s_del_channel(mic_rx_handle);
        mic_rx_handle = NULL;
        return ret;
    }

    ESP_LOGI(TAG, "I2S mic initialized on port %d (BCK:%d, WS:%d, DIN:%d)",
             MIC_I2S_PORT, MIC_BCK_IO, MIC_WS_IO, MIC_DATA_IN_IO);
    return ESP_OK;
}

esp_err_t mic_i2s_read(int32_t *buffer, size_t samples, TickType_t timeout) {
    if (!mic_rx_handle) {
        ESP_LOGE(TAG, "I2S mic not initialized");
        return ESP_ERR_INVALID_STATE;
    }

    size_t bytes_read = 0;
    size_t bytes_to_read = samples * sizeof(int32_t);
    // timeout в ticks, но i2s_channel_read ожидает таймаут в миллисекундах?
    // Функция принимает TickType_t, так что передаём как есть.
    esp_err_t ret = i2s_channel_read(mic_rx_handle, buffer, bytes_to_read, &bytes_read, timeout);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "i2s_channel_read failed: %d", ret);
        return ret;
    }
    if (bytes_read != bytes_to_read) {
        ESP_LOGW(TAG, "Short read: %d/%d", bytes_read, bytes_to_read);
        return ESP_ERR_INVALID_SIZE;
    }
    return ESP_OK;
}

void mic_i2s_deinit(void) {
    if (mic_rx_handle) {
        i2s_channel_disable(mic_rx_handle);
        i2s_del_channel(mic_rx_handle);
        mic_rx_handle = NULL;
        ESP_LOGI(TAG, "I2S mic deinitialized");
    }
}