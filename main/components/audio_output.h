#ifndef __AUDIO_OUTPUT_H__
#define __AUDIO_OUTPUT_H__

#include "driver/i2s_std.h"
#include "config.h"

// Инициализация аудиовыхода
void audio_output_init(void);

// Воспроизведение тестового тона
void audio_play_test_tone(int duration_ms);

// Воспроизведение ноты
void audio_play_note(float frequency, int duration_ms);

#endif // __AUDIO_OUTPUT_H__