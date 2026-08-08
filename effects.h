#pragma once
#include <stdint.h>
#include "game_types.h"

#define NUM_STARS 12
#define MAX_PARTICLES 45

struct Star {
    float x;
    int y;
    float speed;
};

struct Particle {
    float x, y;
    float vx, vy;
    uint16_t color;
    int life;
    bool active;
};

extern Star stars[NUM_STARS];
extern Particle particles[MAX_PARTICLES];

extern int shake_timer;
extern int shake_intensity;

void init_starfield();
void update_and_draw_starfield(int old_shake_x, int old_shake_y, int new_shake_x, int new_shake_y);

void trigger_screenshake(int intensity, int duration_frames);
void update_screenshake();

void spawn_explosion(int x, int y);
void spawn_thruster_plume(int x, int y);
void update_and_draw_particles(int old_shake_x, int old_shake_y, int new_shake_x, int new_shake_y);
