#include "effects.h"
#include "graphics.h"
#include "audio.h"
#include "constants.h"
#include "sprites.h"
#include <stdlib.h>
#include <string.h>

Star stars[NUM_STARS];
CelestialBody background_celestial = {140.0f, 35.0f, 0.05f, 1, true};
ShootingStar background_comet = {0.0f, 0.0f, 0.0f, 0.0f, 0, false};
Particle particles[MAX_PARTICLES];

int shake_timer = 0;
int shake_intensity = 0;

void init_starfield() {
    for (int i = 0; i < NUM_STARS; i++) {
        stars[i].x = rand() % SCREEN_WIDTH;
        stars[i].y = 20 + rand() % (SCREEN_HEIGHT - 35);
        
        if (i < 8) {
            stars[i].layer = 0; // Layer 0: Distant deep space
            stars[i].speed = 0.2f;
        } else if (i < 18) {
            stars[i].layer = 1; // Layer 1: Midfield
            stars[i].speed = 0.6f;
        } else {
            stars[i].layer = 2; // Layer 2: Fast foreground streaks
            stars[i].speed = 1.4f;
        }
    }

    background_celestial = {150.0f, 35.0f, 0.04f, 1, true};
    background_comet.active = false;
}

void update_and_draw_starfield(int old_shake_x, int old_shake_y, int new_shake_x, int new_shake_y) {
    int prev_draw_shake_x = draw_shake_offset_x;
    int prev_draw_shake_y = draw_shake_offset_y;
    draw_shake_offset_x = 0;
    draw_shake_offset_y = 0;

    static int frame_counter = 0;
    frame_counter++;

    // --- 1. Distant Subtle Celestials (Spiral Galaxy & Gas Giant) ---
    if (background_celestial.active) {
        int cx = (int)background_celestial.x;
        int cy = (int)background_celestial.y;
        int c_size = (background_celestial.type == 1) ? 10 : 8;

        // Erase old celestial position
        erase_sprite(cx, cy, c_size, c_size);

        // Move celestial ultra-slowly leftward (far background depth)
        background_celestial.x -= 0.03f;

        // Draw new celestial position
        int new_cx = (int)background_celestial.x;
        if (new_cx > -12 && new_cx < SCREEN_WIDTH) {
            for (int r = 0; r < c_size; r++) {
                for (int c = 0; c < c_size; c++) {
                    uint16_t col = (background_celestial.type == 1) ? 
                                    sprite_celestial_gas_giant[r][c] : 
                                    sprite_celestial_galaxy[r][c];
                    if (col != C_TRANS) {
                        draw_rect(new_cx + c, cy + r, 1, 1, col);
                    }
                }
            }
        }

        // Respawn celestial when off-screen
        if (background_celestial.x < -15) {
            background_celestial.x = SCREEN_WIDTH + 50;
            background_celestial.y = 24 + (rand() % (SCREEN_HEIGHT - 65));
            background_celestial.type = (rand() % 2 == 0) ? 1 : 2;
        }
    }

    // --- 2. Soft Ambient Cosmic Nebulae Clouds (Ultra-slow ambient space dust) ---
    static struct NebulaCloud {
        float x;
        int y;
        uint16_t color;
    } nebulae[4] = {
        {30.0f, 40, 0x1805},  // Faint dark violet
        {85.0f, 75, 0x0108},  // Faint dark teal
        {120.0f, 110, 0x2005}, // Faint deep indigo
        {60.0f, 130, 0x0845}   // Faint slate
    };

    for (int n = 0; n < 4; n++) {
        int nx = (int)nebulae[n].x;
        int ny = nebulae[n].y;
        draw_rect(nx, ny, 2, 2, COLOR_BLACK);
        nebulae[n].x -= 0.06f;
        if (nebulae[n].x < -5) {
            nebulae[n].x = SCREEN_WIDTH + 10;
            nebulae[n].y = 22 + (rand() % (SCREEN_HEIGHT - 45));
        }
        int new_nx = (int)nebulae[n].x;
        if (new_nx >= 0 && new_nx < SCREEN_WIDTH - 2) {
            draw_rect(new_nx, ny, 2, 2, nebulae[n].color);
        }
    }

    // --- 3. Parallax 3-Layer Subtle Starfield (All 1x1 Dots, Low Luminance) ---
    for (int i = 0; i < NUM_STARS; i++) {
        int old_sx = (int)stars[i].x;
        int sy = stars[i].y;

        // Erase old star using old shake offset
        draw_rect(old_sx + old_shake_x, sy + old_shake_y, 1, 1, COLOR_BLACK);

        // Move star leftward
        stars[i].x -= stars[i].speed;

        // Wrap around
        if (stars[i].x < 0) {
            stars[i].x = SCREEN_WIDTH - 1;
            stars[i].y = 20 + rand() % (SCREEN_HEIGHT - 35);
        }

        int new_sx = (int)stars[i].x;

        // Layer color & subtle twinkling logic (low-luminance dim space hues)
        uint16_t star_color;
        if (stars[i].layer == 0) {
            // Layer 0: Distant Deep Space - dim violet/blue pulse
            star_color = ((frame_counter + i * 7) / 20 % 2 == 0) ? 0x2808 : 0x1804;
        } else if (stars[i].layer == 1) {
            // Layer 1: Midfield - dim teal
            star_color = ((frame_counter + i * 3) / 14 % 2 == 0) ? 0x0190 : 0x0110;
        } else {
            // Layer 2: Foreground - soft dim silver dot
            star_color = 0x634E;
        }

        // Draw new star
        draw_rect(new_sx + new_shake_x, sy + new_shake_y, 1, 1, star_color);
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

FloatingText floating_texts[MAX_FLOATING_TEXTS];

void spawn_particle_single(float x, float y, float vx, float vy, uint16_t color) {
    for (int i = 0; i < MAX_PARTICLES; i++) {
        if (!particles[i].active) {
            particles[i].active = true;
            particles[i].x = x;
            particles[i].y = y;
            particles[i].vx = vx;
            particles[i].vy = vy;
            particles[i].color = color;
            particles[i].life = 8 + (rand() % 8);
            break;
        }
    }
}

void spawn_floating_text(float x, float y, const char *str, uint16_t color) {
    for (int i = 0; i < MAX_FLOATING_TEXTS; i++) {
        if (!floating_texts[i].active) {
            floating_texts[i].active = true;
            floating_texts[i].x = x;
            floating_texts[i].y = y;
            strncpy(floating_texts[i].text, str, 15);
            floating_texts[i].text[15] = '\0';
            floating_texts[i].color = color;
            floating_texts[i].life = 25; // ~0.8s float
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

void update_and_draw_floating_text(int old_shake_x, int old_shake_y, int new_shake_x, int new_shake_y) {
    int prev_draw_shake_x = draw_shake_offset_x;
    int prev_draw_shake_y = draw_shake_offset_y;
    draw_shake_offset_x = 0;
    draw_shake_offset_y = 0;

    for (int i = 0; i < MAX_FLOATING_TEXTS; i++) {
        if (floating_texts[i].active) {
            int tw = strlen(floating_texts[i].text) * 6;
            // Erase old text bounding box
            draw_rect((int)floating_texts[i].x + old_shake_x, (int)floating_texts[i].y + old_shake_y, tw, 8, COLOR_BLACK);

            // Float upward slightly
            floating_texts[i].y -= 0.5f;
            floating_texts[i].life--;

            if (floating_texts[i].life <= 0 || floating_texts[i].y < 18.0f) {
                floating_texts[i].active = false;
            } else {
                // Draw new text
                draw_text((int)floating_texts[i].x + new_shake_x, (int)floating_texts[i].y + new_shake_y, floating_texts[i].text, floating_texts[i].color);
            }
        }
    }

    draw_shake_offset_x = prev_draw_shake_x;
    draw_shake_offset_y = prev_draw_shake_y;
}
