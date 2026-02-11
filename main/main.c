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

// Глобальные переменные
//static uint8_t brightness = INITIAL_BRIGHTNESS;
//static bool pattern_active = true;

void app_main() {
    // Инициализация компонентов
  //  led_strip_handle_t led_strip = led_controller_init();
    button_controller_init();
    audio_output_init();

    // Установка начальной яркости
  //  led_controller_set_brightness(led_strip, brightness);

    // Проигрываем тестовый звук при запуске
    //audio_play_test_tone(300);
    
    // Основной цикл
    while (1) {
        // Проверка нажатий кнопок
        bool btn1_pressed = button_controller_check_press(&button1_state);
        bool btn2_pressed = button_controller_check_press(&button2_state);
        
        // Обработка кнопок
        if (btn1_pressed) {
            ESP_LOGI(MAIN_TAG, "Brightness UP");
          //  brightness = (brightness <= 245) ? brightness + 10 : 255;
           // led_controller_set_brightness(led_strip, brightness);
           // pattern_active = false;
            audio_play_note(440, 100); // Короткий звук
        }
        
        if (btn2_pressed) {
            ESP_LOGI(MAIN_TAG, "Brightness DOWN");
        //    brightness = (brightness >= 10) ? brightness - 10 : 0;
        //    led_controller_set_brightness(led_strip, brightness);
        //    pattern_active = false;
            audio_play_note(330, 100); // Короткий звук
        }
        
        // Запуск тестового паттерна
    //    if (pattern_active) {
    //        led_controller_test_pattern(led_strip, &pattern_active);
   //         led_controller_clear(led_strip);
   //     }
        
        // Короткая задержка для цикла
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}