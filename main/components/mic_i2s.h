#pragma once
#include <stdint.h>
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

esp_err_t mic_i2s_init(void);
esp_err_t mic_i2s_read(int32_t *buffer, size_t samples, TickType_t timeout);
void mic_i2s_deinit(void);