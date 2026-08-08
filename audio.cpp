// audio.cpp — Minimal active buzzer: single short click for all sounds, no BGM noise
#include "audio.h"
#include "constants.h"
#include "pico/stdlib.h"
#include "hardware/gpio.h"

#define BUZZER_PIN 15

uint32_t audio_timer = 0;

void init_audio() {
    gpio_init(BUZZER_PIN);
    gpio_set_dir(BUZZER_PIN, GPIO_OUT);
    gpio_put(BUZZER_PIN, 0);
}

void stop_tone() {
    gpio_put(BUZZER_PIN, 0);
    audio_timer = 0;
}

void play_sfx(uint32_t duration_frames) {
    gpio_put(BUZZER_PIN, 1);
    audio_timer = duration_frames;
}

// One universal short click — active buzzer sounds identical at all frequencies.
// Duration is capped at 2 frames (~60ms) so sounds never drag on.
void play_tone(int /*freq*/, uint32_t /*duration*/) {
    if (audio_timer == 0) {          // Don't interrupt a playing click
        gpio_put(BUZZER_PIN, 1);
        audio_timer = 2;             // ~60ms — short, snappy, not irritating
    }
}

void update_audio() {
    if (audio_timer > 0) {
        audio_timer--;
        if (audio_timer == 0) {
            gpio_put(BUZZER_PIN, 0);
        }
    }
}

// Silent BGM — no random background beeping
void update_bgm(int /*frame*/) {}