#include "audio_output.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "math.h"
#include "esp_heap_caps.h"


#include <string.h>
#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include "nvs_flash.h"
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

#include <string.h>


static const char *TAG = AUDIO_TAG;
static i2s_chan_handle_t tx_chan = NULL; 
static int volume = 20;

    audio_pipeline_handle_t pipeline;
    audio_element_handle_t http_stream_reader, i2s_stream_writer, mp3_decoder;
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
    // esp_log_level_set("*", ESP_LOG_WARN);
    // esp_log_level_set(TAG, ESP_LOG_DEBUG);

    ESP_LOGI(TAG, "[ 1 ] Start audio codec chip");
    audio_board_handle_t board_handle = audio_board_init();
    audio_hal_ctrl_codec(board_handle->audio_hal, AUDIO_HAL_CODEC_MODE_DECODE, AUDIO_HAL_CTRL_START);

    ESP_LOGI(TAG, "[2.0] Create audio pipeline for playback");
    audio_pipeline_cfg_t pipeline_cfg = DEFAULT_AUDIO_PIPELINE_CONFIG();
    pipeline = audio_pipeline_init(&pipeline_cfg);
    mem_assert(pipeline);

    ESP_LOGI(TAG, "[2.1] Create http stream to read data");
    http_stream_cfg_t http_cfg = HTTP_STREAM_CFG_DEFAULT();
    http_stream_reader = http_stream_init(&http_cfg);

    ESP_LOGI(TAG, "[2.2] Create i2s stream to write data to codec chip");
    i2s_stream_cfg_t i2s_cfg = I2S_STREAM_CFG_DEFAULT();
    i2s_cfg.type = AUDIO_STREAM_WRITER;
    i2s_cfg.use_alc = true;      // ← ВКЛЮЧАЕМ АППАРАТНУЮ ГРОМКОСТЬ
    i2s_cfg.volume = 32;         // ← НАЧАЛЬНАЯ ГРОМКОСТЬ (0…64, 32 = 0dB)
    i2s_stream_writer = i2s_stream_init(&i2s_cfg);

    ESP_LOGI(TAG, "[2.3] Create mp3 decoder to decode mp3 file");
    mp3_decoder_cfg_t mp3_cfg = DEFAULT_MP3_DECODER_CONFIG();
    mp3_decoder = mp3_decoder_init(&mp3_cfg);

    ESP_LOGI(TAG, "[2.4] Register all elements to audio pipeline");
    audio_pipeline_register(pipeline, http_stream_reader, "http");
    audio_pipeline_register(pipeline, mp3_decoder,        "mp3");
    audio_pipeline_register(pipeline, i2s_stream_writer,  "i2s");

    ESP_LOGI(TAG, "[2.5] Link it together http_stream-->mp3_decoder-->i2s_stream-->[codec_chip]");
    const char *link_tag[3] = {"http", "mp3", "i2s"};
    audio_pipeline_link(pipeline, &link_tag[0], 3);

    ESP_LOGI(TAG, "[2.6] Set up  uri (http as http_stream, mp3 as mp3 decoder, and default output is i2s)");
    audio_element_set_uri(http_stream_reader, "https://dl.espressif.com/dl/audio/ff-16b-2c-44100hz.mp3");

    ESP_LOGI(TAG, "[ 3 ] Start and wait for Wi-Fi network");
    esp_periph_config_t periph_cfg = DEFAULT_ESP_PERIPH_SET_CONFIG();
    esp_periph_set_handle_t set = esp_periph_set_init(&periph_cfg);
    periph_wifi_cfg_t wifi_cfg = {
        .wifi_config.sta.ssid = CONFIG_WIFI_SSID,
        .wifi_config.sta.password = CONFIG_WIFI_PASSWORD,
    };
    esp_periph_handle_t wifi_handle = periph_wifi_init(&wifi_cfg);
    esp_periph_start(set, wifi_handle);
    periph_wifi_wait_for_connected(wifi_handle, portMAX_DELAY);
    
    // Example of using an audio event -- START
    ESP_LOGI(TAG, "[ 4 ] Set up  event listener");
    audio_event_iface_cfg_t evt_cfg = AUDIO_EVENT_IFACE_DEFAULT_CFG();
    audio_event_iface_handle_t evt = audio_event_iface_init(&evt_cfg);

    ESP_LOGI(TAG, "[4.1] Listening event from all elements of pipeline");
    audio_pipeline_set_listener(pipeline, evt);

    ESP_LOGI(TAG, "[4.2] Listening event from peripherals");
    audio_event_iface_set_listener(esp_periph_set_get_event_iface(set), evt);

    ESP_LOGI(TAG, "[ 5 ] Start audio_pipeline");
    audio_pipeline_run(pipeline);
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

void audio_adjust_volume(int vol) {
    volume+=vol;

    if  (volume < MIN_VOLUME)
        volume = MIN_VOLUME;
    if  (volume > MAX_VOLUME)
        volume = MAX_VOLUME;
    
    i2s_alc_volume_set(i2s_stream_writer, volume);
}