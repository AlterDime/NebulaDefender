#include "game.h"
#include "graphics.h"
#include "effects.h"
#include "audio.h"
#include <stdlib.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/flash.h"
#include "hardware/sync.h"
#include "hardware/gpio.h"

// Define game entities
Player player;
Bullet bullets[MAX_BULLETS];
Enemy enemies[MAX_ENEMIES];
PowerUpItem power_up;
BossBullet boss_bullets[MAX_BOSS_BULLETS];
EnemyBullet enemy_bullets[MAX_ENEMY_BULLETS];

// HUD & State definitions
int score = 0;
int lives = 3;
bool shield_active = false;
bool double_shot_active = false;
GameState current_state = STATE_INTRO;
bool state_changed = true;
int high_score = 0;

// Boss State definitions
bool boss_active = false;
BossType current_boss_type = BOSS_MOTHERSHIP;
int boss_encounter_count = 0;
float boss_x = 130.0f;
float boss_y = 60.0f;
float boss_vy = 0.6f;
int boss_hp = 0;
int boss_max_hp = 15;
int boss_shoot_timer = 0;
int last_boss_score = 0;

// Smart Bomb / Charge Blast state definitions
bool charge_blast_enabled = false;
int button_hold_frames = 0;
int radial_blast_timer = 0;
int radial_blast_x = 0;
int radial_blast_y = 0;
int radial_blast_drawn_radius = 0;
int enemy_spawn_delay_timer = 0;

// Tutorial flags
bool tutorial_shield_done = false;
bool tutorial_double_done = false;
bool tutorial_bomb_done = false;

// Flash Target: Last 4KB sector of 2MB Flash (Memory map start: XIP_BASE)
#define FLASH_TARGET_OFFSET (2048 * 1024 - 4096)

void load_high_score() {
    const uint8_t *flash_target_contents = (const uint8_t *) (XIP_BASE + FLASH_TARGET_OFFSET);
    int val = *(const int *)flash_target_contents;
    if (val >= 0 && val <= 999) {
        high_score = val;
    } else {
        high_score = 0;
    }
}

void save_high_score(int score) {
    if (score <= high_score) return;
    high_score = score;

    uint8_t buf[FLASH_PAGE_SIZE] = {0};
    *(int *)buf = high_score;

    uint32_t ints = save_and_disable_interrupts();
    flash_range_erase(FLASH_TARGET_OFFSET, FLASH_SECTOR_SIZE);
    flash_range_program(FLASH_TARGET_OFFSET, buf, FLASH_PAGE_SIZE);
    restore_interrupts(ints);
}

void show_tutorial_overlay(const char *title, const char *desc) {
    // Draw solid dark box in center
    draw_rect(10, 40, 108, 80, COLOR_BLACK);
    
    // Draw borders
    draw_rect(10, 40, 108, 2, C_CYAN);
    draw_rect(10, 118, 108, 2, C_CYAN);
    draw_rect(10, 40, 2, 80, C_CYAN);
    draw_rect(116, 40, 2, 80, C_CYAN);

    // Draw "NEW POWERUP!" label
    draw_text(32, 48, "NEW POWERUP", COLOR_YELLOW);

    // Draw Title
    int title_x = 64 - (strlen(title) * 6) / 2;
    draw_text(title_x, 66, title, COLOR_WHITE);

    // Draw Description
    int desc_x = 64 - (strlen(desc) * 6) / 2;
    draw_text(desc_x, 82, desc, C_CYAN);

    // Draw dismiss cue
    draw_text(23, 102, "TAP TO DISMISS", COLOR_GREEN);

    // Wait for button press & release
    sleep_ms(300); // Debounce
    while (gpio_get(BUTTON_PIN) == 0) {
        sleep_ms(10);
    }
    while (gpio_get(BUTTON_PIN) == 1) {
        sleep_ms(10);
    }
    while (gpio_get(BUTTON_PIN) == 0) {
        sleep_ms(10);
    }
    sleep_ms(200); // Debounce
    
    // Clear screen so that gameplay loop redraws cleanly
    draw_rect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, COLOR_BLACK);
}

EnemyType get_progressive_enemy_type(int current_score) {
    int max_types = 1; // Default: Scout only
    if (current_score >= 45) {
        max_types = 5;
    } else if (current_score >= 30) {
        max_types = 4;
    } else if (current_score >= 15) {
        max_types = 3;
    } else if (current_score >= 5) {
        max_types = 2;
    }

    int roll = rand() % 100;
    if (max_types == 5) {
        if (roll < 15) return ENEMY_SCOUT;
        else if (roll < 30) return ENEMY_BOMBER;
        else if (roll < 50) return ENEMY_CHARGER;
        else if (roll < 75) return ENEMY_DIVER;
        else return ENEMY_SHOOTER;
    } else if (max_types == 4) {
        if (roll < 20) return ENEMY_SCOUT;
        else if (roll < 40) return ENEMY_BOMBER;
        else if (roll < 70) return ENEMY_CHARGER;
        else return ENEMY_DIVER;
    } else if (max_types == 3) {
        if (roll < 30) return ENEMY_SCOUT;
        else if (roll < 60) return ENEMY_BOMBER;
        else return ENEMY_CHARGER;
    } else if (max_types == 2) {
        if (roll < 50) return ENEMY_SCOUT;
        else return ENEMY_BOMBER;
    }
    return ENEMY_SCOUT;
}

void reset_game() {
    player.y = SCREEN_HEIGHT / 2.0f;
    player.velocity = 0;
    score = 0;
    lives = 3;
    shield_active = false;
    double_shot_active = false;
    power_up.active = false;

    // Reset Boss parameters
    boss_active = false;
    boss_encounter_count = 0;
    boss_hp = 0;
    for (int k = 0; k < MAX_BOSS_BULLETS; k++) {
        boss_bullets[k].active = false;
    }
    boss_shoot_timer = 0;
    last_boss_score = 0;

    // Reset Charge Blast parameters
    charge_blast_enabled = false;
    button_hold_frames = 0;
    radial_blast_timer = 0;
    radial_blast_drawn_radius = 0;
    enemy_spawn_delay_timer = 0;

    for (int i = 0; i < MAX_BULLETS; i++) {
        bullets[i].active = false;
    }

    for (int i = 0; i < MAX_ENEMY_BULLETS; i++) {
        enemy_bullets[i].active = false;
    }

    for (int i = 0; i < MAX_ENEMIES; i++) {
        enemies[i].active = true;
        enemies[i].x = SCREEN_WIDTH + 30 + (i * 55);
        enemies[i].base_y = 20 + rand() % (SCREEN_HEIGHT - 40);
        enemies[i].y = enemies[i].base_y;
        enemies[i].speed = 0.8f + (rand() % 80) / 100.0f;
        enemies[i].dived = false;
        enemies[i].shoot_cooldown = 40 + rand() % 80;
        
        enemies[i].type = get_progressive_enemy_type(score);
        if (enemies[i].type == ENEMY_CHARGER) {
            enemies[i].speed *= 1.4f;
        }
    }

    for (int p = 0; p < MAX_PARTICLES; p++) {
        particles[p].active = false;
    }
    init_starfield();
}

void fire_bullet() {
    // Play retro high-frequency synth shoot sound
    play_tone(880, 2);

    if (double_shot_active) {
        int spawned = 0;
        for (int i = 0; i < MAX_BULLETS; i++) {
            if (!bullets[i].active) {
                bullets[i].x = PLAYER_X + player.width;
                bullets[i].y = (int)player.y + (spawned == 0 ? 1 : 6);
                bullets[i].active = true;
                spawned++;
                if (spawned == 2) break;
            }
        }
    } else {
        for (int i = 0; i < MAX_BULLETS; i++) {
            if (!bullets[i].active) {
                bullets[i].x = PLAYER_X + player.width;
                bullets[i].y = (int)player.y + player.height / 2;
                bullets[i].active = true;
                break;
            }
        }
    }
}

bool new_best_achieved = false;

void trigger_gameover() {
    new_best_achieved = (score > high_score);
    save_high_score(score);
    current_state = STATE_GAMEOVER;
    state_changed = true;
}
