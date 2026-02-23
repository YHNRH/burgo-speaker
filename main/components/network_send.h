#pragma once
#include <stdint.h>
#include <stddef.h>
#include "esp_err.h"
void send_wav_via_http(int32_t *pcm32, size_t num_samples);
void send_raw_audio(int32_t *data, size_t samples);
esp_err_t send_audio_get_response(int32_t *pcm32, size_t samples,
                                  uint8_t **out_buf, size_t *out_size);