#pragma once
#include <stdint.h>
#include "game_types.h"
#include "constants.h"

// Define constants
#define MAX_BULLETS 24
#define MAX_ENEMIES 6
#define MAX_BOSS_BULLETS 12
#define MAX_ENEMY_BULLETS 4
#define MAX_GEMS 10

// Gem item struct definition is in game_types.h
extern Player player;
extern Bullet bullets[MAX_BULLETS];
extern Enemy enemies[MAX_ENEMIES];
extern PowerUpItem power_up;
extern BossBullet boss_bullets[MAX_BOSS_BULLETS];
extern EnemyBullet enemy_bullets[MAX_ENEMY_BULLETS];
extern Gem gems[MAX_GEMS];

// HUD & State
extern int weapon_level;       // Stacking weapon upgrade level (1 to 4)
extern int auto_fire_timer;    // Cooldown timer for fire rate
extern GameMode selected_mode;
extern int score;
extern int lives;
extern bool shield_active;
extern int shield_timer;
extern bool shield_permanent;
extern bool double_shot_active;
extern int double_shot_timer;
extern bool double_shot_permanent;
extern bool spread_shot_active;
extern int spread_shot_timer;
extern bool spread_shot_permanent;
extern HelperDrone helper_drone;
extern bool laser_grid_active;
extern int laser_grid_timer;
extern bool overload_active;
extern int overload_timer;
extern bool missile_active;
extern int missile_timer;
extern int bomb_count;
extern int combo_count;
extern int combo_timer;
extern bool new_best_announced;
extern bool game_paused;
extern GameState current_state;
extern bool state_changed;
extern int high_score;

// Persistent Stats & Achievements
extern int total_bosses_defeated;
extern int highest_combo;
extern int total_games_played;

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
extern bool tutorial_life_done;
extern bool tutorial_spread_done;
extern bool tutorial_drone_done;
extern bool tutorial_lasergrid_done;
extern bool tutorial_overload_done;
extern bool tutorial_missile_done;
extern bool new_best_achieved;

// Functions
void load_high_score();
void save_high_score(int score);
void save_all_stats();
void load_all_stats();
void reset_game();
void show_tutorial_overlay(PowerUpType type);
EnemyType get_progressive_enemy_type(int current_score);
PowerUpType get_progressive_powerup_type(int current_score);
int get_powerup_duration(int current_score);
void fire_bullet();
void trigger_gameover();
