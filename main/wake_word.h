#pragma once
#include <stdbool.h>
#include <stdint.h>
#include "esp_err.h"

esp_err_t wake_word_init(const char *model_name);
bool wake_word_process(int16_t *samples, size_t sample_count);
int wake_word_get_chunk_size(void);