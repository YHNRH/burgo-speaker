// // #include <stdio.h>
// // #include <stdlib.h>
// // #include "freertos/FreeRTOS.h"
// // #include "freertos/task.h"
// // #include "model_path.h"
// // #include "string.h"
// // #include "hiesp.h"
// // #include "hilexin.h"
// // #include "esp_wn_iface.h"
// // #include "esp_wn_models.h"
// // #include "driver/i2s.h"

// // #include <WiFi.h>
// // #include <HTTPClient.h>
// // #include <Audio.h>
// // #include "esp_wifi.h"

// // //=== Настройки =================================//
// // #define MIC_DATA_PIN   13
// // #define MIC_CLOCK_PIN  2
// // #define MIC_LR_PIN     15
// // const char *TARGET_MODEL = "mycroft";

// // const char* ssid = "linked";
// // const char* password = "waLL69*9";

// // const char* audioUrls[] = {
// //   "http://192.168.1.52:3001/api/sounds/start.mp3",
// //   "http://192.168.1.52:3001/api/sounds/finish.mp3"
// //   // "http://codeskulptor-demos.commondatastorage.googleapis.com/pang/arrow.mp3",
// //   // "http://codeskulptor-demos.commondatastorage.googleapis.com/pang/pop.mp3"
// // };
// // const int TRACK_COUNT = 2; // Количество треков
// // int currentTrackIndex = 0; // Индекс текущего трека

// // #define I2S_DOUT  17
// // #define I2S_BCLK  18
// // #define I2S_LRC   16

// // Audio audio;
// // bool isPlaying = false;
// // unsigned long lastPlayTime = 0;
// // const unsigned long playInterval = 5000; // 5 секунд

// // //=== Конфигурация I2S ==========================//
// // void setup_i2s() {
// //     i2s_config_t i2s_config = {
// //         .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
// //         .sample_rate = 16000,
// //         .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT, // Исправлено на 32 бита
// //         .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
// //         .communication_format = I2S_COMM_FORMAT_STAND_I2S,
// //         .intr_alloc_flags = 0,
// //         .dma_buf_count = 8,
// //         .dma_buf_len = 512, // Увеличено для стабильности
// //         .use_apll = false
// //     };

// //     i2s_pin_config_t pin_config = {
// //         .bck_io_num = MIC_CLOCK_PIN,
// //         .ws_io_num = MIC_LR_PIN,
// //         .data_out_num = -1,
// //         .data_in_num = MIC_DATA_PIN
// //     };

// //     i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL);
// //     i2s_set_pin(I2S_NUM_0, &pin_config);
// // }

// // void app_main(void) {
// //     // 1. Инициализация микрофона
// //     setup_i2s();
    
// //  WiFi.mode(WIFI_STA);
// // esp_wifi_set_max_tx_power(40);
// // WiFi.setSleep(false);
// //   WiFi.begin(ssid, password);
// //   while (WiFi.status() != WL_CONNECTED) {
// //     delay(500);
// //     Serial.print(".");
// //   }
// //   Serial.println("\nWiFi подключён!");

// //   audio.setPinout(I2S_BCLK, I2S_LRC, I2S_DOUT);
// //   audio.setVolume(30);





// //     // 2. Загрузка модели wake word
// //     srmodel_list_t *models = esp_srmodel_init("model");
// //     if(!models) {
// //         printf("Error loading models!\n");
// //         return;
// //     }

// //     char *model_name = esp_srmodel_filter(models, ESP_WN_PREFIX, TARGET_MODEL);
// //     if(!model_name) {
// //         printf("Model %s not found!\n", TARGET_MODEL);
// //         return;
// //     }

// //     // 3. Инициализация обработчика
// //     esp_wn_iface_t *wakenet = (esp_wn_iface_t*)esp_wn_handle_from_name(model_name);
// //     if(!wakenet) {
// //         printf("WN handler init failed!\n");
// //         return;
// //     }

// //     model_iface_data_t *model_data = wakenet->create(model_name, DET_MODE_90); // Понижен порог

// //     // 4. Настройка аудиобуфера
// //     int audio_chunksize = wakenet->get_samp_chunksize(model_data) * sizeof(int16_t);
// //     int16_t *buffer = (int16_t *)malloc(audio_chunksize);
// //     if(!buffer) {
// //         printf("Memory allocation failed!\n");
// //         return;
// //     }

// //     printf("\n=== Start listening for '%s' ===\n", TARGET_MODEL);

// //     // 5. Основной цикл распознавания
// //     while(1) {
// //         size_t bytes_read = 0;
// //         i2s_read(I2S_NUM_0, buffer, audio_chunksize, &bytes_read, portMAX_DELAY);

// //         // Проверка на wake word
// //         wakenet_state_t state = wakenet->detect(model_data, buffer);
        
// //         // Отладочный вывод
// //         static int counter = 0;
// //         if(++counter % 100 == 0) {
// //             printf("State: %d, Free mem: %ld bytes\n", 
// //                 state, 
// //                 esp_get_free_heap_size());
// //         }
        
// //         if(state == WAKENET_DETECTED) {
// //             printf("\n=== WAKE WORD DETECTED! ===\n");
// //         }
// //     }
// // }


// // void startPlayback() {
// //   if (WiFi.status() == WL_CONNECTED) {
// //     Serial.println("Starting playback...");

// //     audio.stopSong(); // Останавливаем предыдущее воспроизведение
// //     delay(500); // Добавить задержку для завершения остановки
// //     Serial.printf("Now playing: Track %d\n", currentTrackIndex + 1);

// //     audio.connecttohost(audioUrls[currentTrackIndex]);
    
// //     isPlaying = true;
// //     lastPlayTime = millis();
// //   }
// // }

// // void audio_info(const char *info) {
// //   Serial.print("info: "); Serial.println(info);
// //   if (strstr(info, "End of webstream")) {
// //     isPlaying = false;
// //   }
// // }

// // void audio_id3data(const char *info) {
// //   Serial.print("id3: "); Serial.println(info);
// // }

// // void audio_eof_mp3(const char *info) {
// //   Serial.print("eof: "); Serial.println(info);
// //   isPlaying = false;
// // }














#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "components/led_controller.h"
#include "components/button_controller.h"
#include "components/audio_output.h"
#include "components/config.h"
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

#if (ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(4, 1, 0))
#include "esp_netif.h"
#else
#include "tcpip_adapter.h"
#endif


// Глобальные переменные
static uint8_t brightness = INITIAL_BRIGHTNESS;
//static bool pattern_active = true;

void app_main() {
    // Инициализация компонентов
    led_strip_handle_t led_strip = led_controller_init();
    button_controller_init();
    audio_output_init();

    // Установка начальной яркости
    led_controller_set_brightness(led_strip, brightness);

    // Проигрываем тестовый звук при запуске
    //audio_play_test_tone(300);
    
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES) {
        // NVS partition was truncated and needs to be erased
        // Retry nvs_flash_init
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
#if (ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(4, 1, 0))
    ESP_ERROR_CHECK(esp_netif_init());
#else
    tcpip_adapter_init();
#endif
    
    // Основной цикл
    while (1) {
        // Проверка нажатий кнопок
        bool btn1_pressed = button_controller_check_press(&button1_state);
        bool btn2_pressed = button_controller_check_press(&button2_state);
        
        // Обработка кнопок
        if (btn1_pressed) {
            ESP_LOGI(MAIN_TAG, "Volume UP");
            audio_adjust_volume(5);
           // audio_play_note(440, 100); // Короткий звук
        }
        
        if (btn2_pressed) {
            ESP_LOGI(MAIN_TAG, "Volume DOWN");
            audio_adjust_volume(-5);
            // audio_play_note(330, 100); // Короткий звук
        }
        
        // Запуск тестового паттерна
    //    if (pattern_active) {
    //        led_controller_test_pattern(led_strip, &pattern_active);
   //         led_controller_clear(led_strip);
   //     }

        //    audio_event_iface_msg_t msg;
        // esp_err_t ret = audio_event_iface_listen(evt, &msg, portMAX_DELAY);
        // if (ret != ESP_OK) {
        //     ESP_LOGE(TAG, "[ * ] Event interface error : %d", ret);
        //     continue;
        // }

        // if (msg.source_type == AUDIO_ELEMENT_TYPE_ELEMENT
        //     && msg.source == (void *) mp3_decoder
        //     && msg.cmd == AEL_MSG_CMD_REPORT_MUSIC_INFO) {
        //     audio_element_info_t music_info = {0};
        //     audio_element_getinfo(mp3_decoder, &music_info);

        //     ESP_LOGI(TAG, "[ * ] Receive music info from mp3 decoder, sample_rates=%d, bits=%d, ch=%d",
        //              music_info.sample_rates, music_info.bits, music_info.channels);

        //     i2s_stream_set_clk(i2s_stream_writer, music_info.sample_rates, music_info.bits, music_info.channels);
        //     continue;
        // }

        // /* Stop when the last pipeline element (i2s_stream_writer in this case) receives stop event */
        // if (msg.source_type == AUDIO_ELEMENT_TYPE_ELEMENT && msg.source == (void *) i2s_stream_writer
        //     && msg.cmd == AEL_MSG_CMD_REPORT_STATUS
        //     && (((int)msg.data == AEL_STATUS_STATE_STOPPED) || ((int)msg.data == AEL_STATUS_STATE_FINISHED))) {
        //     ESP_LOGW(TAG, "[ * ] Stop event received");
        //     break;
        // }
        
        // Короткая задержка для цикла
        vTaskDelay(pdMS_TO_TICKS(10));
    }

    // Example of using an audio event -- END

    // ESP_LOGI(TAG, "[ 6 ] Stop audio_pipeline");
    // audio_pipeline_stop(pipeline);
    // audio_pipeline_wait_for_stop(pipeline);
    // audio_pipeline_terminate(pipeline);

    // /* Terminate the pipeline before removing the listener */
    // audio_pipeline_unregister(pipeline, http_stream_reader);
    // audio_pipeline_unregister(pipeline, i2s_stream_writer);
    // audio_pipeline_unregister(pipeline, mp3_decoder);

    // audio_pipeline_remove_listener(pipeline);

    // /* Stop all peripherals before removing the listener */
    // esp_periph_set_stop_all(set);
    // audio_event_iface_remove_listener(esp_periph_set_get_event_iface(set), evt);

    // /* Make sure audio_pipeline_remove_listener & audio_event_iface_remove_listener are called before destroying event_iface */
    // audio_event_iface_destroy(evt);

    // /* Release all resources */
    // audio_pipeline_deinit(pipeline);
    // audio_element_deinit(http_stream_reader);
    // audio_element_deinit(i2s_stream_writer);
    // audio_element_deinit(mp3_decoder);
    // esp_periph_set_destroy(set);
}


// void audio_init(void)
// {
//     ESP_LOGI(TAG, "Initializing I2S stream...");
    
//     i2s_stream_cfg_t i2s_cfg = I2S_STREAM_CFG_DEFAULT();
//     i2s_cfg.need_expand = false;
//     i2s_cfg.type = AUDIO_STREAM_WRITER;
//     i2s_cfg.use_alc = true;           // включаем громкость
//     i2s_cfg.volume = 1;             // 0..64
    
//     //i2s_cfg.i2s_config.sample_rate = SAMPLE_RATE;
//     // i2s_cfg.i2s_config.bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT;
//     // i2s_cfg.i2s_config.channel_format = I2S_CHANNEL_FMT_ONLY_LEFT;
//     // i2s_cfg.i2s_config.communication_format = I2S_COMM_FORMAT_STAND_I2S;
//     // i2s_cfg.i2s_port = I2S_NUM_0;
    
//     // ПИНЫ I2S — ЗАДАЁМ ЧЕРЕЗ pin_config
//     // i2s_pin_config_t pin_config = {
//     //     .bck_io_num = 26,
//     //     .ws_io_num = 25,
//     //     .data_out_num = 32,
//     //     .data_in_num = -1
//     // };
//     // i2s_cfg.pin_config = &pin_config;
    
//     i2s_stream_writer = i2s_stream_init(&i2s_cfg);
//     ESP_LOGI(TAG, "I2S initialized, volume = %d", i2s_cfg.volume);
// }

// void play_tone(float frequency, int duration_ms, int volume)
// {
//     // Устанавливаем громкость
//     esp_err_t err = i2s_alc_volume_set(i2s_stream_writer, volume);
//     if (err != ESP_OK) {
//         ESP_LOGE(TAG, "ALC volume set failed: %s", esp_err_to_name(err));
//     }
//     ESP_LOGI(TAG, "ALC volume set failed: %s", esp_err_to_name(err));
    
//     int num_samples = (SAMPLE_RATE * duration_ms) / 1000;
    
//     // DMA-память обязательна
//     int16_t *samples = heap_caps_malloc(num_samples * sizeof(int16_t), MALLOC_CAP_DMA);
//     if (!samples) {
//         ESP_LOGE(TAG, "Failed to allocate DMA memory");
//         return;
//     }
    
//     // Генерация синуса
//     for (int i = 0; i < num_samples; i++) {
//         double angle = 2.0 * M_PI * frequency * i / SAMPLE_RATE;
//         samples[i] = (int16_t)(AMPLITUDE * sin(angle));
//     }
    
//     // ★★★ ИСПРАВЛЕНО: ТРИ АРГУМЕНТА, БЕЗ bytes_written И ТАЙМАУТА ★★★
//     audio_element_err_t ret = audio_element_output(
//         i2s_stream_writer,
//         (char*)samples,
//         num_samples * sizeof(int16_t)
//     );
    
//     // if (ret != AUDIO_ELEMENT_ERR_OK) {
//     //     ESP_LOGE(TAG, "audio_element_output failed: %d", ret);
//     // }
    
//     // Тишина для выталкивания буфера
//     int16_t *silence = heap_caps_calloc(256, sizeof(int16_t), MALLOC_CAP_DMA);
//     if (silence) {
//         audio_element_output(i2s_stream_writer, (char*)silence, 256 * sizeof(int16_t));
//         free(silence);
//     }
    
//     free(samples);
// }