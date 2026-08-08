#pragma once
#include <stdint.h>
#include "game_types.h"

#define NUM_STARS 24
#define MAX_PARTICLES 45

struct Star {
    float x;
    int y;
    float speed;
    int layer; // 0 = Distant Twinkle, 1 = Midfield, 2 = Foreground Streak
};

struct CelestialBody {
    float x;
    float y;
    float speed;
    int type; // 1 = Ringed Gas Giant, 2 = Silver Moon
    bool active;
};

struct ShootingStar {
    float x;
    float y;
    float vx;
    float vy;
    int life;
    bool active;
};

struct Particle {
    float x, y;
    float vx, vy;
    uint16_t color;
    int life;
    bool active;
};

#define MAX_FLOATING_TEXTS 6

struct FloatingText {
    float x, y;
    char text[16];
    uint16_t color;
    int life;
    bool active;
};

extern Star stars[NUM_STARS];
extern CelestialBody background_celestial;
extern ShootingStar background_comet;
extern Particle particles[MAX_PARTICLES];
extern FloatingText floating_texts[MAX_FLOATING_TEXTS];

extern int shake_timer;
extern int shake_intensity;

void init_starfield();
void update_and_draw_starfield(int old_shake_x, int old_shake_y, int new_shake_x, int new_shake_y);

void trigger_screenshake(int intensity, int duration_frames);
void update_screenshake();

void spawn_explosion(int x, int y);
void spawn_thruster_plume(int x, int y);
void spawn_particle_single(float x, float y, float vx, float vy, uint16_t color);
void spawn_floating_text(float x, float y, const char *str, uint16_t color);
void update_and_draw_particles(int old_shake_x, int old_shake_y, int new_shake_x, int new_shake_y);
void update_and_draw_floating_text(int old_shake_x, int old_shake_y, int new_shake_x, int new_shake_y);
