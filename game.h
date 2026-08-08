#pragma once
#include <stdint.h>
#include "game_types.h"
#include "constants.h"

// Define constants
#define MAX_BULLETS 8
#define MAX_ENEMIES 3
#define MAX_BOSS_BULLETS 3
#define MAX_ENEMY_BULLETS 4

// Game entities
extern Player player;
extern Bullet bullets[MAX_BULLETS];
extern Enemy enemies[MAX_ENEMIES];
extern PowerUpItem power_up;
extern BossBullet boss_bullets[MAX_BOSS_BULLETS];
extern EnemyBullet enemy_bullets[MAX_ENEMY_BULLETS];

// HUD & State
extern int score;
extern int lives;
extern bool shield_active;
extern bool double_shot_active;
extern GameState current_state;
extern bool state_changed;
extern int high_score;

// Boss State
extern bool boss_active;
extern BossType current_boss_type;
extern int boss_encounter_count;
extern float boss_x;
extern float boss_y;
extern float boss_vy;
extern int boss_hp;
extern int boss_max_hp;
extern int boss_shoot_timer;
extern int last_boss_score;

// Smart Bomb / Charge Blast state
extern bool charge_blast_enabled;
extern int button_hold_frames;
extern int radial_blast_timer;
extern int radial_blast_x;
extern int radial_blast_y;
extern int radial_blast_drawn_radius;
extern int enemy_spawn_delay_timer;

// Tutorial state flags
extern bool tutorial_shield_done;
extern bool tutorial_double_done;
extern bool tutorial_bomb_done;
extern bool new_best_achieved;

// Functions
void load_high_score();
void save_high_score(int score);
void reset_game();
void show_tutorial_overlay(const char *title, const char *desc);
EnemyType get_progressive_enemy_type(int current_score);
void fire_bullet();
void trigger_gameover();
