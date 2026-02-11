#ifndef __BUTTON_CONTROLLER_H__
#define __BUTTON_CONTROLLER_H__

#include "driver/gpio.h"
#include "config.h"

// Структура для состояния кнопки
typedef struct {
    gpio_num_t gpio;
    bool last_state;
    bool pressed;
    uint32_t last_press_time;
} button_state_t;

// Инициализация кнопок
void button_controller_init(void);

// Проверка нажатия кнопки (возвращает true при новом нажатии)
bool button_controller_check_press(button_state_t *sensor);

// Глобальные состояния кнопок
extern button_state_t button1_state;
extern button_state_t button2_state;

#endif // __BUTTON_CONTROLLER_H__