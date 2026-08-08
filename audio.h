#pragma once
#include <stdint.h>

extern uint32_t audio_timer;

void init_audio();
void stop_tone();
void play_sfx(uint32_t duration_frames);
void play_tone(int frequency_unused, uint32_t duration_frames);
void update_audio();
void update_bgm(int frame);