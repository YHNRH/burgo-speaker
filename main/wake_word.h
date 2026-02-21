// wake_word.h
#pragma once
#include <stdbool.h>
#include <stdint.h>
#include "esp_err.h"
#include "esp_afe_sr_iface.h"
#include "esp_afe_sr_models.h"
// Старый API (без AFE)
esp_err_t wake_word_init(const char *model_name);
bool wake_word_process(int16_t *samples, size_t sample_count);
int wake_word_get_chunk_size(void);

extern esp_afe_sr_data_t *afe_data;
extern esp_afe_sr_iface_t *afe_handle;
extern int afe_chunk_size;

// Новый API (с AFE)
esp_err_t afe_init(const char *model_name);
bool afe_process(int16_t *samples, size_t sample_count, int16_t **processed, size_t *processed_samples);
void afe_deinit(void);