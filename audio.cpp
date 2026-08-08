#include "audio.h"
#include "constants.h"
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/pwm.h"
#include "hardware/clocks.h"

#define BUZZER_PIN 15

uint32_t audio_timer = 0;

void stop_tone() {
    uint slice_num = pwm_gpio_to_slice_num(BUZZER_PIN);
    pwm_set_enabled(slice_num, false);
    gpio_init(BUZZER_PIN); // Reset GP15 to inert state to prevent click hums
}

void play_tone(uint32_t frequency, uint32_t duration_frames) {
    if (frequency == 0) {
        stop_tone();
        return;
    }
    gpio_set_function(BUZZER_PIN, GPIO_FUNC_PWM);
    uint slice_num = pwm_gpio_to_slice_num(BUZZER_PIN);
    uint chan = pwm_gpio_to_channel(BUZZER_PIN);

    uint32_t clock_freq = clock_get_hz(clk_sys);
    uint32_t wrap = clock_freq / frequency;
    pwm_set_wrap(slice_num, wrap);
    pwm_set_chan_level(slice_num, chan, wrap / 2); // 50% duty cycle square wave
    pwm_set_enabled(slice_num, true);

    audio_timer = duration_frames;
}

void update_audio() {
    if (audio_timer > 0) {
        audio_timer--;
        if (audio_timer == 0) {
            stop_tone();
        }
    }
}

void update_bgm(int frame) {
    if (audio_timer > 0) return; // Active SFX overrides background music
    static const uint16_t bassline[16] = {
        131, 0, 131, 0, 165, 0, 196, 0,
        131, 0, 147, 0, 165, 0, 175, 0
    };
    int step = (frame / 6) % 16;
    uint16_t note = bassline[step];
    if (note > 0 && (frame % 6 == 0)) {
        play_tone(note, 2);
    }
}
