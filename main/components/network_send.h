#pragma once
#include <stdint.h>
#include <stddef.h>
void send_wav_via_http(int32_t *pcm32, size_t num_samples);
void send_raw_audio(int32_t *data, size_t samples);