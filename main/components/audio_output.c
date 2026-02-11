#include "audio_output.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "math.h"
#include "esp_heap_caps.h"
#include <string.h>
static i2s_chan_handle_t tx_chan = NULL; 
// void audio_output_init(void) {
//     // Конфигурация I2S
//     i2s_config_t i2s_config = {
//         .mode = I2S_MODE_MASTER | I2S_MODE_TX,
//         .sample_rate = I2S_SAMPLE_RATE,
//         .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
//         .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
//         .communication_format = I2S_COMM_FORMAT_STAND_I2S,
//         .dma_buf_count = 6,
//         .dma_buf_len = 128,
//         .use_apll = false,
//         .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1
//     };

//     // Конфигурация пинов
//     i2s_pin_config_t pin_config = {
//         .bck_io_num = I2S_BCK_GPIO,
//         .ws_io_num = I2S_WS_GPIO,
//         .data_out_num = I2S_DATA_OUT_GPIO,
//         .data_in_num = -1  // Не используется
//     };

//     // Установка I2S драйвера
//     ESP_ERROR_CHECK(i2s_driver_install(I2S_NUM, &i2s_config, 0, NULL));
//     ESP_ERROR_CHECK(i2s_set_pin(I2S_NUM, &pin_config));
    
//     ESP_LOGI(AUDIO_TAG, "Audio output initialized");
// }

void audio_output_init(void) {
    // 1. Базовая конфигурация канала
    i2s_chan_config_t chan_cfg = {
        .id = I2S_NUM_0,                     // обычно I2S_NUM_0
        .role = I2S_ROLE_MASTER,             // мастер
        .dma_desc_num = 6,                  // количество DMA-дескрипторов
        .dma_frame_num = 128,              // размер одного фрейма в сэмплах
        .auto_clear = true,                // автоматически очищать буфер при нехватке данных
    };
    ESP_ERROR_CHECK(i2s_new_channel(&chan_cfg, &tx_chan, NULL));

    // 2. Конфигурация стандартного режима I2S (Philips)
    i2s_std_config_t std_cfg = {
        .clk_cfg = {
            .sample_rate_hz = I2S_SAMPLE_RATE,
            .clk_src = I2S_CLK_SRC_DEFAULT, // или I2S_CLK_SRC_APLL
            .mclk_multiple = I2S_MCLK_MULTIPLE_256,
        },
        .slot_cfg = {
            .data_bit_width = I2S_DATA_BIT_WIDTH_16BIT,
            .slot_bit_width = I2S_SLOT_BIT_WIDTH_16BIT,
            .slot_mode = I2S_SLOT_MODE_MONO,      // моно
            .slot_mask = I2S_STD_SLOT_LEFT,       // выводим на левый канал
            .ws_width = I2S_SLOT_BIT_WIDTH_16BIT, // ширина WS (обычно = slot_bit_width)
            .ws_pol = false,                     // низкий уровень = начало кадра
            .bit_shift = true,                  // MSB first
            .left_align = true,                // выравнивание по левому краю
            .big_endian = false,
            .bit_order_lsb = false,
        },
        .gpio_cfg = {
            .bclk = I2S_BCK_GPIO,
            .ws = I2S_WS_GPIO,
            .dout = I2S_DATA_OUT_GPIO,
            .din = I2S_GPIO_UNUSED,
            .invert_flags = {
                .bclk_inv = false,
                .ws_inv = false,
            //    .dout_inv = false,
            //    .din_inv = false,
            },
        },
    };
    ESP_ERROR_CHECK(i2s_channel_init_std_mode(tx_chan, &std_cfg));

    // 3. Включение канала (начало передачи)
    ESP_ERROR_CHECK(i2s_channel_enable(tx_chan));

    ESP_LOGI(AUDIO_TAG, "Audio output initialized (new I2S driver)");
}

// void audio_play_note(float frequency, int duration_ms) {
//     // Рассчитываем количество сэмплов
//     int num_samples = (I2S_SAMPLE_RATE * duration_ms) / 1000;
//     int16_t *samples = malloc(num_samples * sizeof(int16_t));
    
//     if (!samples) {
//         ESP_LOGE(AUDIO_TAG, "Failed to allocate memory for samples");
//         return;
//     }

//     // Генерируем синусоидальный сигнал
//     for (int i = 0; i < num_samples; i++) {
//         double angle = 2.0 * M_PI * frequency * i / I2S_SAMPLE_RATE;
//         samples[i] = (int16_t)(AUDIO_AMPLITUDE * sin(angle));
//     }

//     // Отправляем данные в I2S
//     size_t bytes_written = 0;
//     i2s_write(I2S_NUM, samples, num_samples * sizeof(int16_t), &bytes_written, portMAX_DELAY);
    
//     free(samples);
// }

void audio_play_note(float frequency, int duration_ms) {
    int num_samples = (I2S_SAMPLE_RATE * duration_ms) / 1000;
    
    // Выделяем DMA-совместимую память (обязательно!)
    int16_t *samples = heap_caps_malloc(num_samples * sizeof(int16_t), MALLOC_CAP_DMA);
    if (!samples) {
        ESP_LOGE(AUDIO_TAG, "Failed to allocate DMA memory for samples");
        return;
    }

    // Генерация синуса
    for (int i = 0; i < num_samples; i++) {
        double angle = 2.0 * M_PI * frequency * i / I2S_SAMPLE_RATE;
        samples[i] = (int16_t)(AUDIO_AMPLITUDE * sin(angle));
    }

    // Синхронная запись – функция вернёт управление только когда все данные
    // будут скопированы во внутренний DMA-буфер.
    size_t bytes_written = 0;
    esp_err_t ret = i2s_channel_write(tx_chan, samples, num_samples * sizeof(int16_t), 
                                      &bytes_written, portMAX_DELAY);
    if (ret != ESP_OK) {
        ESP_LOGE(AUDIO_TAG, "I2S write failed: %s", esp_err_to_name(ret));
    }

    // Освобождаем буфер (данные уже в DMA, можно безопасно удалять)
    free(samples);
}

void audio_play_test_tone(int duration_ms) {
    // Проигрываем последовательность нот
    audio_play_note(261.63, duration_ms/3); // До
    audio_play_note(329.63, duration_ms/3); // Ми
    audio_play_note(392.00, duration_ms/3); // Соль
}