#include "game.h"
#include "graphics.h"
#include "sprites.h"
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
GameMode selected_mode = MODE_NORMAL;
int score = 0;
int lives = 3;
bool shield_active = false;
int shield_timer = 0;
bool shield_permanent = false;
bool double_shot_active = false;
int double_shot_timer = 0;
bool double_shot_permanent = false;
bool spread_shot_active = false;
int spread_shot_timer = 0;
bool spread_shot_permanent = false;
HelperDrone helper_drone = {0.0f, 0.0f, 0.0f, false, 0, 0};
BlackHole black_hole = {64.0f, 85.0f, false, 0, 0};
bool overload_active = false;
int overload_timer = 0;
int bomb_count = 0;
int combo_count = 0;
int combo_timer = 0;
bool new_best_announced = false;
bool game_paused = false;
GameState current_state = STATE_INTRO;
bool state_changed = true;
int high_score = 0;

// Persistent Stats & Achievements
int total_bosses_defeated = 0;
int highest_combo = 0;
int total_games_played = 0;

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
bool tutorial_life_done = false;
bool tutorial_spread_done = false;
bool tutorial_drone_done = false;
bool tutorial_blackhole_done = false;
bool tutorial_overload_done = false;
bool new_best_achieved = false;

// Flash Target: Last 4KB sector of 2MB Flash (Memory map start: XIP_BASE)
#define FLASH_TARGET_OFFSET (2048 * 1024 - 4096)

void load_all_stats() {
    const int *flash_vals = (const int *) (XIP_BASE + FLASH_TARGET_OFFSET);
    if (flash_vals[0] >= 0 && flash_vals[0] <= 9999) high_score = flash_vals[0]; else high_score = 0;
    if (flash_vals[1] >= 0 && flash_vals[1] <= 9999) total_bosses_defeated = flash_vals[1]; else total_bosses_defeated = 0;
    if (flash_vals[2] >= 0 && flash_vals[2] <= 999) highest_combo = flash_vals[2]; else highest_combo = 0;
    if (flash_vals[3] >= 0 && flash_vals[3] <= 9999) total_games_played = flash_vals[3]; else total_games_played = 0;
}

void save_all_stats() {
    uint8_t buf[FLASH_PAGE_SIZE] = {0};
    int *data = (int *)buf;
    data[0] = high_score;
    data[1] = total_bosses_defeated;
    data[2] = highest_combo;
    data[3] = total_games_played;

    uint32_t ints = save_and_disable_interrupts();
    flash_range_erase(FLASH_TARGET_OFFSET, FLASH_SECTOR_SIZE);
    flash_range_program(FLASH_TARGET_OFFSET, buf, FLASH_PAGE_SIZE);
    restore_interrupts(ints);
}

void load_high_score() { load_all_stats(); }

void save_high_score(int score) {
    if (score > high_score) high_score = score;
    save_all_stats();
}

void show_tutorial_overlay(PowerUpType type) {
    // 1. Play ascending audio fanfare melody
    play_tone(523, 2); sleep_ms(35);
    play_tone(659, 2); sleep_ms(35);
    play_tone(784, 2); sleep_ms(35);
    play_tone(1046, 5);

    uint16_t theme_color = C_CYAN;
    const char *title = "";
    const char *line1 = "";
    const char *line2 = "";
    const uint16_t (*sprite_data)[8] = sprite_powerup_shield;

    if (type == POWERUP_SHIELD) {
        theme_color = C_CYAN;
        title = "SHIELD MATRIX";
        line1 = "ABSORBS 1 HIT";
        line2 = "DEFLECTS DAMAGE";
        sprite_data = sprite_powerup_shield;
    } else if (type == POWERUP_DOUBLE) {
        theme_color = COLOR_YELLOW;
        title = "DUAL CANNON";
        line1 = "TWIN LASER SALVO";
        line2 = "DOUBLE FIREPOWER";
        sprite_data = sprite_powerup_double;
    } else if (type == POWERUP_BOMB) {
        theme_color = COLOR_MAGENTA;
        title = "SMART BOMB";
        line1 = "HOLD TO CHARGE";
        line2 = "WIPES ENEMIES";
        sprite_data = sprite_powerup_bomb;
    } else if (type == POWERUP_LIFE) {
        theme_color = COLOR_GREEN;
        title = "REPAIR SHIP";
        line1 = "+1 EXTRA LIFE";
        line2 = "RESTORES HULL";
        sprite_data = sprite_powerup_life;
    } else if (type == POWERUP_SPREAD) {
        theme_color = 0xFD20; // Orange
        title = "SPREAD TRIPLE";
        line1 = "3-WAY LASER SHOT";
        line2 = "WIDE COVERAGE";
        sprite_data = sprite_powerup_spread;
    } else if (type == POWERUP_DRONE) {
        theme_color = C_CYAN;
        title = "HELPER DRONE";
        line1 = "AUTONOMOUS BOT";
        line2 = "AUTO TARGET FIRE";
        sprite_data = sprite_powerup_drone;
    } else if (type == POWERUP_BLACKHOLE) {
        theme_color = COLOR_MAGENTA;
        title = "BLACK HOLE";
        line1 = "SINGULARITY VOID";
        line2 = "PULLS & CRUSHES";
        sprite_data = sprite_powerup_blackhole;
    } else if (type == POWERUP_OVERLOAD) {
        theme_color = COLOR_RED;
        title = "HYPER OVERLOAD";
        line1 = "HYPER RAPID FIRE";
        line2 = "AUTO LASER STORM";
        sprite_data = sprite_powerup_overload;
    }

    // 2. Render Card Background (Navy dark panel with double neon border)
    // Card bounds: x=8, y=28, w=112, h=108
    draw_rect(8, 28, 112, 108, 0x0821); // Dark navy fill

    // Double outer neon border
    draw_rect(8, 28, 112, 2, theme_color);       // Top edge
    draw_rect(8, 134, 112, 2, theme_color);      // Bottom edge
    draw_rect(8, 28, 2, 108, theme_color);       // Left edge
    draw_rect(118, 28, 2, 108, theme_color);     // Right edge

    // Inner shadow frame line
    draw_rect(10, 30, 108, 1, 0x0155);
    draw_rect(10, 133, 108, 1, 0x0155);
    draw_rect(10, 30, 1, 104, 0x0155);
    draw_rect(117, 30, 1, 104, 0x0155);

    // Decorative corner accent blocks
    draw_rect(8, 28, 5, 5, theme_color);
    draw_rect(115, 28, 5, 5, theme_color);
    draw_rect(8, 131, 5, 5, theme_color);
    draw_rect(115, 131, 5, 5, theme_color);

    // 3. Header Banner: "NEW POWERUP!"
    draw_rect(24, 33, 80, 11, 0x18C3);
    draw_rect(24, 33, 80, 1, theme_color);
    draw_rect(24, 43, 80, 1, theme_color);
    draw_text(31, 35, "NEW POWERUP!", COLOR_YELLOW);

    // 4. Draw 2x Scaled Power-Up Sprite in center of card (16x16 icon)
    // Frame box around icon (x=53, y=47, w=22, h=20)
    draw_rect(53, 47, 22, 20, COLOR_BLACK);
    draw_rect(53, 47, 22, 1, theme_color);
    draw_rect(53, 66, 22, 1, theme_color);
    draw_rect(53, 47, 1, 20, theme_color);
    draw_rect(74, 47, 1, 20, theme_color);

    for (int row = 0; row < 8; row++) {
        for (int col = 0; col < 8; col++) {
            uint16_t color = sprite_data[row][col];
            if (color != C_TRANS) {
                draw_rect(56 + col * 2, 49 + row * 2, 2, 2, color);
            }
        }
    }

    // 5. Title & Description Details
    int title_x = 64 - (strlen(title) * 6) / 2;
    draw_text(title_x, 70, title, COLOR_WHITE);
    draw_text(title_x, 71, title, COLOR_WHITE); // bold offset

    int line1_x = 64 - (strlen(line1) * 6) / 2;
    draw_text(line1_x, 85, line1, theme_color);

    int line2_x = 64 - (strlen(line2) * 6) / 2;
    draw_text(line2_x, 97, line2, 0x7BEF);

    // 6. Dismiss Cue: "TAP TO PLAY"
    draw_rect(23, 115, 82, 13, 0x0155);
    draw_text(31, 118, "TAP TO PLAY", COLOR_GREEN);

    // Wait for single tap release + press (clean 1-tap dismiss)
    sleep_ms(250); // Initial debounce
    while (gpio_get(BUTTON_PIN) == 0) {
        sleep_ms(10);
    }
    while (gpio_get(BUTTON_PIN) == 1) {
        sleep_ms(10);
    }
    sleep_ms(150); // Debounce
    
    // Clear screen so that gameplay loop redraws cleanly
    draw_rect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, COLOR_BLACK);
}

EnemyType get_progressive_enemy_type(int current_score) {
    int roll = rand() % 100;
    if (current_score >= 120) {
        if (roll < 8) return ENEMY_SCOUT;
        else if (roll < 16) return ENEMY_BOMBER;
        else if (roll < 25) return ENEMY_CHARGER;
        else if (roll < 35) return ENEMY_DIVER;
        else if (roll < 45) return ENEMY_SHOOTER;
        else if (roll < 55) return ENEMY_SHIELDED;
        else if (roll < 65) return ENEMY_SWARMER;
        else if (roll < 75) return ENEMY_TURRET;
        else if (roll < 83) return ENEMY_PHANTOM;
        else if (roll < 90) return ENEMY_MINELAYER;
        else if (roll < 95) return ENEMY_BEAMER;
        else return ENEMY_COMMANDER;
    } else if (current_score >= 80) {
        if (roll < 10) return ENEMY_SCOUT;
        else if (roll < 20) return ENEMY_BOMBER;
        else if (roll < 32) return ENEMY_CHARGER;
        else if (roll < 45) return ENEMY_DIVER;
        else if (roll < 58) return ENEMY_SHOOTER;
        else if (roll < 70) return ENEMY_SHIELDED;
        else if (roll < 80) return ENEMY_SWARMER;
        else if (roll < 88) return ENEMY_TURRET;
        else if (roll < 94) return ENEMY_PHANTOM;
        else return ENEMY_MINELAYER;
    } else if (current_score >= 60) {
        if (roll < 10) return ENEMY_SCOUT;
        else if (roll < 20) return ENEMY_BOMBER;
        else if (roll < 35) return ENEMY_CHARGER;
        else if (roll < 50) return ENEMY_DIVER;
        else if (roll < 65) return ENEMY_SHOOTER;
        else if (roll < 80) return ENEMY_SHIELDED;
        else if (roll < 90) return ENEMY_SWARMER;
        else return ENEMY_TURRET;
    } else if (current_score >= 45) {
        if (roll < 15) return ENEMY_SCOUT;
        else if (roll < 30) return ENEMY_BOMBER;
        else if (roll < 50) return ENEMY_CHARGER;
        else if (roll < 70) return ENEMY_DIVER;
        else if (roll < 85) return ENEMY_SHOOTER;
        else return ENEMY_SHIELDED;
    } else if (current_score >= 30) {
        if (roll < 20) return ENEMY_SCOUT;
        else if (roll < 40) return ENEMY_BOMBER;
        else if (roll < 60) return ENEMY_CHARGER;
        else if (roll < 80) return ENEMY_DIVER;
        else return ENEMY_SHOOTER;
    } else if (current_score >= 15) {
        if (roll < 30) return ENEMY_SCOUT;
        else if (roll < 60) return ENEMY_BOMBER;
        else if (roll < 85) return ENEMY_CHARGER;
        else return ENEMY_DIVER;
    } else if (current_score >= 5) {
        if (roll < 50) return ENEMY_SCOUT;
        else return ENEMY_BOMBER;
    }
    return ENEMY_SCOUT;
}

PowerUpType get_progressive_powerup_type(int current_score) {
    if (current_score < 30) {
        // Early game: only basic powerups (Shield, Double Gun, Bomb, Life). OP items locked!
        int roll = rand() % 4;
        if (roll == 0)      return POWERUP_SHIELD;
        else if (roll == 1) return POWERUP_DOUBLE;
        else if (roll == 2) return POWERUP_BOMB;
        else                return POWERUP_LIFE;
    } else if (current_score < 60) {
        // Mid game: unlocks Spread and Helper Drone
        int roll = rand() % 6;
        if (roll == 0)      return POWERUP_SHIELD;
        else if (roll == 1) return POWERUP_DOUBLE;
        else if (roll == 2) return POWERUP_BOMB;
        else if (roll == 3) return POWERUP_LIFE;
        else if (roll == 4) return POWERUP_SPREAD;
        else                return POWERUP_DRONE;
    } else {
        // Late game: unlocks all powerups including OP Black Hole and Hyper Overload!
        int roll = rand() % 8;
        if (roll == 0)      return POWERUP_SHIELD;
        else if (roll == 1) return POWERUP_DOUBLE;
        else if (roll == 2) return POWERUP_BOMB;
        else if (roll == 3) return POWERUP_SPREAD;
        else if (roll == 4) return POWERUP_DRONE;
        else if (roll == 5) return POWERUP_BLACKHOLE;
        else if (roll == 6) return POWERUP_OVERLOAD;
        else                return POWERUP_LIFE;
    }
}

int get_powerup_duration(int current_score) {
    if (current_score < 30) {
        return 900;  // 30 Seconds in Early Game
    } else if (current_score < 60) {
        return 1800; // 60 Seconds (1 Minute) in Mid Game
    } else {
        return 2700; // 90 Seconds (1.5 Minutes) in Late Game
    }
}

void reset_game() {
    player.y = SCREEN_HEIGHT / 2.0f;
    player.velocity = 0;
    score = 0;
    lives = 3;
    shield_active = false;
    shield_timer = 0;
    shield_permanent = false;
    double_shot_active = false;
    double_shot_timer = 0;
    double_shot_permanent = false;
    spread_shot_active = false;
    spread_shot_timer = 0;
    spread_shot_permanent = false;
    helper_drone = {0.0f, 0.0f, 0.0f, false, 0, 0};
    black_hole = {64.0f, 85.0f, false, 0, 0};
    overload_active = false;
    overload_timer = 0;
    bomb_count = 0;
    combo_count = 0;
    combo_timer = 0;
    new_best_announced = false;
    game_paused = false;
    power_up.active = false;
    power_up.duration_frames = 0;
    total_games_played++;

    // Reset Boss parameters
    boss_active = false;
    boss_encounter_count = 0;
    boss_hp = 0;
    for (int k = 0; k < MAX_BOSS_BULLETS; k++) {
        boss_bullets[k].active = false;
    }
    boss_shoot_timer = 0;
    last_boss_score = 0;

    // Erase boss HP bar area unconditionally on reset
    draw_shake_offset_x = 0;
    draw_shake_offset_y = 0;
    draw_rect(0, 148, 128, 12, COLOR_BLACK);

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
        enemies[i].base_y = 24 + rand() % (SCREEN_HEIGHT - 48);
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

    if (spread_shot_active) {
        // 3x Spread Shot: up to 24 bullets on screen simultaneously (8 salvos of 3)
        int spawned = 0;
        int offsets[3] = {1, 4, 7};
        for (int i = 0; i < 24; i++) {
            if (!bullets[i].active) {
                bullets[i].x = PLAYER_X + player.width;
                bullets[i].y = (int)player.y + offsets[spawned];
                bullets[i].active = true;
                spawned++;
                if (spawned == 3) break;
            }
        }
    } else if (double_shot_active) {
        // 2x Dual Gun: up to 16 bullets on screen simultaneously (8 salvos of 2)
        int spawned = 0;
        for (int i = 0; i < 16; i++) {
            if (!bullets[i].active) {
                bullets[i].x = PLAYER_X + player.width;
                bullets[i].y = (int)player.y + (spawned == 0 ? 1 : 6);
                bullets[i].active = true;
                spawned++;
                if (spawned == 2) break;
            }
        }
    } else {
        // 1x Single Gun: up to 8 bullets on screen simultaneously (8 single shots)
        for (int i = 0; i < 8; i++) {
            if (!bullets[i].active) {
                bullets[i].x = PLAYER_X + player.width;
                bullets[i].y = (int)player.y + player.height / 2;
                bullets[i].active = true;
                break;
            }
        }
    }
}

void trigger_gameover() {
    new_best_achieved = (score > high_score);
    save_high_score(score);
    current_state = STATE_GAMEOVER;
    state_changed = true;
}
