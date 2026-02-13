#ifndef __CONFIG_H__
#define __CONFIG_H__

// Пин для светодиодной ленты
#define LED_GPIO    47
// Количество светодиодов
#define LED_COUNT   3

// Пины для кнопок
#define TOUCH_SENSOR_1_GPIO   42//GPIO_NUM_42  // Кнопка увеличения яркости
#define TOUCH_SENSOR_2_GPIO   41//GPIO_NUM_41   // Кнопка уменьшения яркости

// Дребезг в миллисекундах
#define DEBOUNCE_TIME_MS     50

// Настройки аудио
#define I2S_SAMPLE_RATE     44100
#define I2S_NUM             I2S_NUM_0
#define I2S_BCK_GPIO        1//41
#define I2S_WS_GPIO         3//38
#define I2S_DATA_OUT_GPIO   2//37
#define AUDIO_AMPLITUDE     20000
#define MAX_VOLUME          64
#define MIN_VOLUME         -64

// Начальная яркость
#define INITIAL_BRIGHTNESS   50

// Теги для логирования
#define LED_TAG     "LED_CONTROLLER"
#define BUTTON_TAG  "BUTTON_CONTROLLER"
#define AUDIO_TAG   "AUDIO_OUTPUT"
#define MAIN_TAG    "MAIN_APP"

#endif // __CONFIG_H__