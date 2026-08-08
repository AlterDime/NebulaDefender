#include "effects.h"
#include "graphics.h"
#include "audio.h"
#include "constants.h"
#include <stdlib.h>

Star stars[NUM_STARS];
Particle particles[MAX_PARTICLES];

int shake_timer = 0;
int shake_intensity = 0;

void init_starfield() {
    for (int i = 0; i < NUM_STARS; i++) {
        stars[i].x = rand() % SCREEN_WIDTH;
        stars[i].y = rand() % SCREEN_HEIGHT;
        stars[i].speed = 0.2f + (rand() % 80) / 100.0f; // Parallax speeds
    }
}

void update_and_draw_starfield(int old_shake_x, int old_shake_y, int new_shake_x, int new_shake_y) {
    int prev_draw_shake_x = draw_shake_offset_x;
    int prev_draw_shake_y = draw_shake_offset_y;
    draw_shake_offset_x = 0;
    draw_shake_offset_y = 0;

    for (int i = 0; i < NUM_STARS; i++) {
        // Erase old star using old shake offset
        draw_rect((int)stars[i].x + old_shake_x, stars[i].y + old_shake_y, 1, 1, COLOR_BLACK);

        // Move star leftward
        stars[i].x -= stars[i].speed;

        // Wrap around
        if (stars[i].x < 0) {
            stars[i].x = SCREEN_WIDTH - 1;
            stars[i].y = rand() % SCREEN_HEIGHT;
        }

        // Draw new star using new shake offset
        uint16_t star_color = (stars[i].speed > 0.6f) ? COLOR_WHITE : 0x5AEB;
        draw_rect((int)stars[i].x + new_shake_x, stars[i].y + new_shake_y, 1, 1, star_color);
    }

    draw_shake_offset_x = prev_draw_shake_x;
    draw_shake_offset_y = prev_draw_shake_y;
}

void trigger_screenshake(int intensity, int duration_frames) {
    shake_intensity = intensity;
    shake_timer = duration_frames;
}

void update_screenshake() {
    if (shake_timer > 0) {
        shake_timer--;
        shake_offset_x = (rand() % (shake_intensity * 2 + 1)) - shake_intensity;
        shake_offset_y = (rand() % (shake_intensity * 2 + 1)) - shake_intensity;
    } else {
        shake_offset_x = 0;
        shake_offset_y = 0;
    }
}

void spawn_explosion(int x, int y) {
    // Play retro explosion sound (low frequency rumble)
    play_tone(140, 5);

    for (int i = 0; i < 8; i++) { // Spawn 8 blast particles
        for (int p = 0; p < MAX_PARTICLES; p++) {
            if (!particles[p].active) {
                particles[p].x = x + 4;
                particles[p].y = y + 4;
                // Random velocities
                particles[p].vx = ((rand() % 200) - 100) / 40.0f;
                particles[p].vy = ((rand() % 200) - 100) / 40.0f;
                particles[p].color = (rand() % 3 == 0) ? COLOR_RED : ((rand() % 2 == 0) ? COLOR_YELLOW : COLOR_WHITE);
                particles[p].life = 8 + rand() % 10;
                particles[p].active = true;
                break;
            }
        }
    }
}

void spawn_thruster_plume(int x, int y) {
    for (int p = 0; p < MAX_PARTICLES; p++) {
        if (!particles[p].active) {
            particles[p].x = x;
            particles[p].y = y + 2 + (rand() % 4); // Rear of the ship
            particles[p].vx = -1.5f - (rand() % 100) / 100.0f; // Float leftwards
            particles[p].vy = ((rand() % 100) - 50) / 100.0f;  // Slight wiggle
            particles[p].color = (rand() % 2 == 0) ? COLOR_YELLOW : C_ORANGE;
            particles[p].life = 4 + rand() % 5;
            particles[p].active = true;
            break;
        }
    }
}

void update_and_draw_particles(int old_shake_x, int old_shake_y, int new_shake_x, int new_shake_y) {
    int prev_draw_shake_x = draw_shake_offset_x;
    int prev_draw_shake_y = draw_shake_offset_y;
    draw_shake_offset_x = 0;
    draw_shake_offset_y = 0;

    for (int i = 0; i < MAX_PARTICLES; i++) {
        if (particles[i].active) {
            // Erase old particle
            draw_rect((int)particles[i].x + old_shake_x, (int)particles[i].y + old_shake_y, 1, 1, COLOR_BLACK);

            // Move particle
            particles[i].x += particles[i].vx;
            particles[i].y += particles[i].vy;
            particles[i].life--;

            if (particles[i].life <= 0 || particles[i].x < 0 || particles[i].x >= SCREEN_WIDTH || particles[i].y < 0 || particles[i].y >= SCREEN_HEIGHT) {
                particles[i].active = false;
            } else {
                // Draw new particle
                draw_rect((int)particles[i].x + new_shake_x, (int)particles[i].y + new_shake_y, 1, 1, particles[i].color);
            }
        }
    }

    draw_shake_offset_x = prev_draw_shake_x;
    draw_shake_offset_y = prev_draw_shake_y;
}
