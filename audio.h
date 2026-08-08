#pragma once
#include <stdint.h>

extern uint32_t audio_timer;

void stop_tone();
void play_tone(uint32_t frequency, uint32_t duration_frames);
void update_audio();
void update_bgm(int frame);
