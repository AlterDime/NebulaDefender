#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "hardware/gpio.h"
#include "hardware/pwm.h"

// Modular Headers
#include "constants.h"
#include "sprites.h"
#include "game_types.h"
#include "graphics.h"
#include "audio.h"
#include "effects.h"
#include "game.h"

#define BUZZER_PIN 15

int main() {
    // Standard SDK initialisation
    stdio_init_all();

    // Initialize SPI0 at 15MHz
    spi_init(SPI_PORT, 15 * 1000 * 1000);
    gpio_set_function(PIN_SCK, GPIO_FUNC_SPI);
    gpio_set_function(PIN_SDA, GPIO_FUNC_SPI);

    // Initialize Control GPIOs
    gpio_init(PIN_CS);
    gpio_set_dir(PIN_CS, GPIO_OUT);
    gpio_put(PIN_CS, 1);

    gpio_init(PIN_DC);
    gpio_set_dir(PIN_DC, GPIO_OUT);
    gpio_put(PIN_DC, 1);

    gpio_init(PIN_RST);
    gpio_set_dir(PIN_RST, GPIO_OUT);
    gpio_put(PIN_RST, 1);

    // Initialize Button GPIO
    gpio_init(BUTTON_PIN);
    gpio_set_dir(BUTTON_PIN, GPIO_IN);
    gpio_pull_up(BUTTON_PIN);

    // Initialize screen
    tft_init();

    // Initialize Buzzer PWM
    gpio_set_function(BUZZER_PIN, GPIO_FUNC_PWM);
    uint slice_num = pwm_gpio_to_slice_num(BUZZER_PIN);
    pwm_set_enabled(slice_num, false);

    // Load High Score from Flash memory
    load_high_score();

    // Start / reset game
    reset_game();

    bool last_button_state = true; // High when not pressed due to pull-up
    bool state_changed = true;     // Forces screen clear on state transitions

    // Store old positions for smooth, flicker-free rendering
    int old_player_y = (int)player.y;
    int old_bullet_x[MAX_BULLETS] = {0};
    int old_bullet_y[MAX_BULLETS] = {0};
    int old_enemy_x[MAX_ENEMIES] = {0};
    int old_enemy_y[MAX_ENEMIES] = {0};
    int old_boss_bullet_x[MAX_BOSS_BULLETS] = {0};
    int old_boss_bullet_y[MAX_BOSS_BULLETS] = {0};
    int old_enemy_bullet_x[MAX_ENEMY_BULLETS] = {0};
    int old_enemy_bullet_y[MAX_ENEMY_BULLETS] = {0};

    int intro_frame_counter = 0;
    int old_intro_ship_y = 65;
    int intro_bullet_x = -1;
    int intro_bullet_y = 0;
    bool boss_was_active = false;
    int gameover_frame_counter = 0;

    int toast_timer = 0;
    const char *toast_text = "";
    uint16_t toast_color = C_CYAN;
    int boss_warning_timer = 0;

    while (true) {
        // Read input (Active LOW)
        bool button_pressed = !gpio_get(BUTTON_PIN);

        // --- STATE: INTRO ---
        if (current_state == STATE_INTRO) {
            if (state_changed) {
                intro_frame_counter = 0;
                intro_bullet_x = -1;

                // === Full background: clean black ===
                draw_rect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, COLOR_BLACK);

                // === Static star field (two layers, drawn once) ===
                // Far layer — small dim blue-white dots
                for (int i = 0; i < 18; i++) {
                    int sx = (i * 37 + 11) % SCREEN_WIDTH;
                    int sy = (i * 23 + 7)  % SCREEN_HEIGHT;
                    draw_rect(sx, sy, 1, 1, 0x2965);
                }
                // Near layer — slightly brighter
                for (int i = 0; i < 10; i++) {
                    int sx = (i * 53 + 29) % SCREEN_WIDTH;
                    int sy = (i * 41 + 19) % SCREEN_HEIGHT;
                    draw_rect(sx, sy, 1, 1, 0x7BEF);
                }

                // === Title block ===
                // "NEBULA" — white bold (double-draw)
                draw_text(34, 28, "NEBULA", COLOR_WHITE);
                draw_text(34, 29, "NEBULA", COLOR_WHITE);
                // "DEFENDER" — cyan bold
                draw_text(22, 40, "DEFENDER", C_CYAN);
                draw_text(22, 41, "DEFENDER", C_CYAN);

                // Thin cyan underline beneath title
                draw_rect(18, 52, SCREEN_WIDTH - 36, 1, C_CYAN);
                draw_rect(18, 53, SCREEN_WIDTH - 36, 1, 0x0155);

                // === BEST SCORE label + 2× big digits ===
                // "BEST" label — dim yellow
                draw_text(52, 82, "BEST", 0x8400);
                // Big digits centred (2× scale = 10px wide + 3px gap, 3 digits = 36px total)
                draw_big_digit(46, 92, (high_score / 100) % 10, COLOR_YELLOW);
                draw_big_digit(59, 92, (high_score / 10)  % 10, COLOR_YELLOW);
                draw_big_digit(72, 92, high_score % 10,          COLOR_YELLOW);

                // Thin separator above best score
                draw_rect(18, 79, SCREEN_WIDTH - 36, 1, 0x2945);
                // Thin separator below best score
                draw_rect(18, 111, SCREEN_WIDTH - 36, 1, 0x2945);

                old_intro_ship_y = 57;
                state_changed = false;
            }

            intro_frame_counter++;

            // === Pulsing border (top + bottom edges only — subtle) ===
            uint16_t border_col = (intro_frame_counter / 25 % 2 == 0) ? C_CYAN : 0x0294;
            draw_rect(0, 0,              SCREEN_WIDTH, 2, border_col);
            draw_rect(0, SCREEN_HEIGHT - 2, SCREEN_WIDTH, 2, border_col);

            // Restore background stars in an erased bounding box
            auto restore_intro_stars = [](int rx, int ry, int rw, int rh) {
                for (int i = 0; i < 18; i++) {
                    int sx = (i * 37 + 11) % SCREEN_WIDTH;
                    int sy = (i * 23 + 7)  % SCREEN_HEIGHT;
                    if (sx >= rx && sx < rx + rw && sy >= ry && sy < ry + rh) {
                        draw_rect(sx, sy, 1, 1, 0x2965);
                    }
                }
                for (int i = 0; i < 10; i++) {
                    int sx = (i * 53 + 29) % SCREEN_WIDTH;
                    int sy = (i * 41 + 19) % SCREEN_HEIGHT;
                    if (sx >= rx && sx < rx + rw && sy >= ry && sy < ry + rh) {
                        draw_rect(sx, sy, 1, 1, 0x7BEF);
                    }
                }
            };

            // Erase prior frame intro ship & plume region cleanly and restore stars
            draw_rect(8, old_intro_ship_y - 2, 22, 12, COLOR_BLACK);
            restore_intro_stars(8, old_intro_ship_y - 2, 22, 12);

            // Erase prior intro bullet if active
            if (intro_bullet_x >= 0) {
                draw_rect(intro_bullet_x - 1, intro_bullet_y - 1, 10, 6, COLOR_BLACK);
                restore_intro_stars(intro_bullet_x - 1, intro_bullet_y - 1, 10, 6);
            }

            int ship_y = 57 + (int)(sinf(intro_frame_counter * 0.12f) * 5.0f);

            // Multi-pixel plasma flame plume (seamlessly attached to ship rear at x=20)
            uint16_t flame1 = (intro_frame_counter % 2 == 0) ? C_ORANGE : COLOR_YELLOW;
            uint16_t flame2 = (intro_frame_counter % 2 == 0) ? COLOR_RED  : C_ORANGE;
            draw_rect(17, ship_y + 3, 3, 2, COLOR_WHITE);
            draw_rect(14, ship_y + 2, 3, 4, flame1);
            draw_rect(11, ship_y + 3, 3, 2, flame2);
            draw_rect(9,  ship_y + 3, 2, 2, COLOR_RED);

            // Ship sprite at x=20
            draw_sprite(20, ship_y, 8, 8, sprite_player);

            // Periodically fire demo laser cannon bolt from ship nose (x=28)
            if (intro_bullet_x < 0 && (intro_frame_counter % 25 == 0)) {
                intro_bullet_x = 28;
                intro_bullet_y = ship_y + 2;
            }

            if (intro_bullet_x >= 0) {
                intro_bullet_x += 5;
                if (intro_bullet_x >= SCREEN_WIDTH - 8) {
                    draw_rect(intro_bullet_x - 1, intro_bullet_y - 1, 10, 6, COLOR_BLACK);
                    restore_intro_stars(intro_bullet_x - 1, intro_bullet_y - 1, 10, 6);
                    intro_bullet_x = -1;
                } else {
                    draw_sprite_bullet(intro_bullet_x, intro_bullet_y);
                }
            }

            old_intro_ship_y = ship_y;

            // === Dot-chase attract strip (Y=76, safely below ship zone) ===
            {
                int lit = (intro_frame_counter / 8) % 7;
                for (int i = 0; i < 7; i++) {
                    uint16_t dot = (i == lit) ? C_CYAN : 0x0294;
                    draw_rect(20 + i * 14, 76, 8, 3, dot);
                }
            }

            // === Mode Display & Selection (Y 118, centered safely below Best Score) ===
            const char *mode_str = (selected_mode == MODE_NORMAL) ? "MODE: NORMAL" : 
                                  ((selected_mode == MODE_HYPER) ? "MODE: HYPER 2X" : "MODE: BOSS RUSH");
            int mode_x = 64 - (strlen(mode_str) * 6) / 2;
            draw_text(mode_x, 118, mode_str, COLOR_YELLOW);

            // === Blinking "TAP TO START" prompt (Y 135) ===
            if ((intro_frame_counter / 20) % 2 == 0) {
                draw_text(28, 135, "TAP TO START", C_CYAN);
            } else {
                draw_rect(28, 135, 72, 8, COLOR_BLACK);
            }

            // Single Button Controls on Intro Screen:
            // - Tap (< 15 frames) -> Start Game
            // - Hold (>= 15 frames) -> Cycle Game Mode (Normal -> Hyper -> Boss Rush)
            static int intro_hold_count = 0;
            if (button_pressed) {
                intro_hold_count++;
            } else {
                if (intro_hold_count > 0) {
                    if (intro_hold_count < 12) {
                        // Short Tap: Start Game!
                        current_state = STATE_PLAYING;
                        state_changed = true;
                        reset_game();
                        draw_rect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, COLOR_BLACK);
                        draw_hud_bar();
                        draw_score(score);
                        draw_lives(lives);
                    } else {
                        // Hold & Release: Cycle Mode!
                        if (selected_mode == MODE_NORMAL)      selected_mode = MODE_HYPER;
                        else if (selected_mode == MODE_HYPER) selected_mode = MODE_BOSSRUSH;
                        else                                  selected_mode = MODE_NORMAL;
                        play_tone(987, 4); // High confirmation chime
                        draw_rect(0, 116, SCREEN_WIDTH, 14, COLOR_BLACK);
                    }
                    intro_hold_count = 0;
                }
            }
            last_button_state = button_pressed;
            sleep_ms(30);
            continue;
        }

        // --- STATE: GAMEOVER ---
        if (current_state == STATE_GAMEOVER) {
            if (state_changed) {
                gameover_frame_counter = 0;
                draw_gameover_base(score, high_score);
                state_changed = false;
            }

            gameover_frame_counter++;
            draw_gameover_overlay(new_best_achieved, gameover_frame_counter);

            // Return to intro on button tap
            if (button_pressed && !last_button_state) {
                current_state = STATE_INTRO;
                state_changed = true;
                new_best_achieved = false;
            }
            last_button_state = button_pressed;
            sleep_ms(30);
            continue;
        }

        // --- STATE: PLAYING ---

        // Detect tap (rising edge) to flap and shoot, and hold to charge radial blast
        if (button_pressed && !last_button_state) {
            player.velocity = JUMP_FORCE;
            fire_bullet();
            button_hold_frames = 1;
        } else if (button_pressed && last_button_state) {
            if (bomb_count > 0) {
                button_hold_frames++;
                // Rising pitch hum cue when fully charged
                if (button_hold_frames == 25) {
                    play_tone(980, 4); 
                }
            }
        } else if (!button_pressed && last_button_state) {
            // Button released! Check if charge blast is triggered
            if (bomb_count > 0 && button_hold_frames >= 25) {
                // Detonate Radial Smart Bomb!
                radial_blast_timer = 8;
                radial_blast_x = PLAYER_X + player.width / 2;
                radial_blast_y = (int)player.y + player.height / 2;

                // Consume 1 bomb from stack
                bomb_count--;

                // Update HUD bomb indicator
                draw_shake_offset_x = 0;
                draw_shake_offset_y = 0;
                draw_score(score);
                draw_shake_offset_x = shake_offset_x;
                draw_shake_offset_y = shake_offset_y;

                // Play massive explosion sound
                play_tone(180, 10);

                // Heavy screenshake
                trigger_screenshake(7, 15);

                // Destroy all standard enemies on board and delay new spawns for 1s
                if (!boss_active) {
                    enemy_spawn_delay_timer = 35; // ~1 second spawn delay
                    for (int i = 0; i < MAX_ENEMIES; i++) {
                        if (enemies[i].active) {
                            spawn_explosion((int)enemies[i].x, (int)enemies[i].y);
                            erase_sprite((int)enemies[i].x, (int)enemies[i].y, 8, 8);
                            enemies[i].active = false; // Turn off immediately
                            score += 1;
                        }
                    }
                } else {
                    // Deal heavy damage (5 HP) to Boss
                    boss_hp -= 5;
                    spawn_explosion((int)boss_x + 8, (int)boss_y + 8);
                    play_tone(880, 4);
                    if (boss_hp <= 0) {
                        boss_active = false;
                        
                        // Clear boss health bar panel from HUD
                        draw_shake_offset_x = 0;
                        draw_shake_offset_y = 0;
                        draw_rect(0, 148, 128, 12, COLOR_BLACK);
                        draw_shake_offset_x = shake_offset_x;
                        draw_shake_offset_y = shake_offset_y;

                        // Clean boss space
                        erase_sprite((int)boss_x - 1, (int)boss_y - 1, 18, 18);
                        for (int k = 0; k < MAX_BOSS_BULLETS; k++) {
                            if (boss_bullets[k].active) {
                                erase_sprite(old_boss_bullet_x[k], old_boss_bullet_y[k], 4, 4);
                                boss_bullets[k].active = false;
                            }
                        }
                        for (int k = 0; k < 4; k++) {
                            spawn_explosion((int)boss_x + rand() % 16, (int)boss_y + rand() % 16);
                        }
                        play_tone(110, 15);
                        score += 5;

                        // Boss power-up drop: 2 to 5 minute durations (No forever/permanent drops)
                        if (!power_up.active) {
                            power_up.active = true;
                            power_up.x = boss_x + 4;
                            power_up.y = boss_y + 4;
                            
                            if (current_boss_type == BOSS_MOTHERSHIP || current_boss_type == BOSS_DREADNOUGHT) {
                                power_up.duration_frames = 3600; // 2 Minutes
                                int roll = rand() % 4;
                                if (roll == 0)      power_up.type = POWERUP_SHIELD;
                                else if (roll == 1) power_up.type = POWERUP_DOUBLE;
                                else if (roll == 2) power_up.type = POWERUP_BOMB;
                                else                power_up.type = POWERUP_LIFE;
                            } else if (current_boss_type == BOSS_VIPER || current_boss_type == BOSS_PHANTOM || current_boss_type == BOSS_TITAN) {
                                power_up.duration_frames = 6300; // 3.5 Minutes
                                power_up.type = (boss_encounter_count % 2 == 1) ? POWERUP_SHIELD : POWERUP_DOUBLE;
                            } else {
                                power_up.duration_frames = 9000; // 5 Minutes
                                int roll = boss_encounter_count % 3;
                                power_up.type = (roll == 0) ? POWERUP_SPREAD : ((roll == 1) ? POWERUP_DOUBLE : POWERUP_SHIELD);
                            }
                            play_tone(1046, 4);
                        }

                        // Re-enable normal enemies
                        for (int i = 0; i < MAX_ENEMIES; i++) {
                            enemies[i].active = true;
                            enemies[i].x = SCREEN_WIDTH + 30 + (i * 55);
                            enemies[i].base_y = 24 + rand() % (SCREEN_HEIGHT - 48);
                            enemies[i].y = enemies[i].base_y;
                            enemies[i].speed = 0.8f + (score * 0.005f) + (rand() % 80) / 100.0f;
                            enemies[i].dived = false;
                            enemies[i].shoot_cooldown = 40 + rand() % 80;
                            enemies[i].type = get_progressive_enemy_type(score);
                            if (enemies[i].type == ENEMY_CHARGER) {
                                enemies[i].speed *= 1.4f;
                            }
                        }
                    }
                }
            }
            button_hold_frames = 0;
        }
        last_button_state = button_pressed;

        // Store previous frame's shake offset for erasing trails accurately
        int o_shake_x = shake_offset_x;
        int o_shake_y = shake_offset_y;

        // Force all updates and boundary erases to use the old shake offset
        draw_shake_offset_x = o_shake_x;
        draw_shake_offset_y = o_shake_y;

        // Trigger Boss Wave every 50 points (or immediately in BOSS RUSH)
        if (!boss_active && (selected_mode == MODE_BOSSRUSH || score >= last_boss_score + 50)) {
            boss_active = true;
            boss_warning_timer = 50;
            last_boss_score = (score / 50) * 50; // Anchor last boss score
            boss_encounter_count++;
            
            // Cycle through all 10 Boss Encounters!
            int mode_mod = boss_encounter_count % 10;
            if      (mode_mod == 1) { current_boss_type = BOSS_MOTHERSHIP; boss_max_hp = 15; }
            else if (mode_mod == 2) { current_boss_type = BOSS_DREADNOUGHT; boss_max_hp = 25; }
            else if (mode_mod == 3) { current_boss_type = BOSS_VIPER;       boss_max_hp = 30; }
            else if (mode_mod == 4) { current_boss_type = BOSS_PHANTOM;     boss_max_hp = 35; }
            else if (mode_mod == 5) { current_boss_type = BOSS_TITAN;       boss_max_hp = 45; }
            else if (mode_mod == 6) { current_boss_type = BOSS_ASTEROID;    boss_max_hp = 55; }
            else if (mode_mod == 7) { current_boss_type = BOSS_DRAGON;      boss_max_hp = 65; }
            else if (mode_mod == 8) { current_boss_type = BOSS_CHRONO;      boss_max_hp = 75; }
            else if (mode_mod == 9) { current_boss_type = BOSS_NEBULA;      boss_max_hp = 85; }
            else                    { current_boss_type = BOSS_OMEGA;       boss_max_hp = 100; }
            boss_hp = boss_max_hp;
            
            boss_x = SCREEN_WIDTH + 10;
            boss_y = SCREEN_HEIGHT / 2.0f - 8;
            boss_vy = 0.6f;
            boss_shoot_timer = 0;

            for (int i = 0; i < MAX_BOSS_BULLETS; i++) {
                boss_bullets[i].active = false;
            }

            // Clear normal enemies off the board (using current erase frame offsets)
            for (int i = 0; i < MAX_ENEMIES; i++) {
                if (enemies[i].active) {
                    erase_sprite(old_enemy_x[i], old_enemy_y[i], 8, 8);
                    enemies[i].active = false;
                }
            }
            
            // Play a triple low siren warning tone
            play_tone(150, 4);
            sleep_ms(150);
            play_tone(150, 4);
            sleep_ms(150);
            play_tone(150, 6);
        }

        // Gameplay thruster embers & combo timer
        static int thruster_ember_cnt = 0;
        thruster_ember_cnt++;
        if (thruster_ember_cnt % 2 == 0) {
            spawn_particle_single(PLAYER_X - 1, (int)player.y + 3 + (rand() % 3), -1.2f, (rand() % 100 - 50) / 100.0f, (rand() % 2 == 0) ? COLOR_YELLOW : C_ORANGE);
        }
        if (combo_timer > 0) {
            combo_timer--;
        }

        // Apply physics: Lift ship on tap, and activate zero-gravity hover lock when holding for >= 5 frames!
        if (bomb_count > 0 && button_hold_frames >= 5) {
            player.velocity = 0.0f;
            // Hover lock: do NOT apply GRAVITY or change player.y while charging!
        } else {
            player.velocity += GRAVITY;
            player.y += player.velocity;
        }

        // Check boundaries (clamp top to Y=19 so player never enters score banner)
        if (player.y < 19.0f) {
            player.y = 19.0f;
            player.velocity = 0;
        }

        // --- Timed Power-Up Logic ---
        if (shield_active && !shield_permanent) {
            shield_timer--;
            if (shield_timer <= 0) {
                shield_active = false;
                play_tone(300, 4); // Low power-down chime
            }
        }
        if (double_shot_active && !double_shot_permanent) {
            double_shot_timer--;
            if (double_shot_timer <= 0) {
                double_shot_active = false;
                play_tone(300, 4); // Low power-down chime
            }
        }
        if (spread_shot_active && !spread_shot_permanent) {
            spread_shot_timer--;
            if (spread_shot_timer <= 0) {
                spread_shot_active = false;
                play_tone(300, 4);
            }
        }

        // Helper Drone Bot
        if (helper_drone.active) {
            helper_drone.timer--;
            if (helper_drone.timer <= 0) {
                helper_drone.active = false;
                play_tone(300, 4);
            } else {
                helper_drone.angle += 0.12f;
                // Forward wingman orbit: always stays in front of player (X = 24..40px), NO player overlap!
                float target_x = (PLAYER_X + 18.0f) + cosf(helper_drone.angle) * 8.0f;
                float target_y = player.y + sinf(helper_drone.angle) * 10.0f;
                
                // Smooth inertial lerp so drone floats with graceful wingman momentum
                helper_drone.x += (target_x - helper_drone.x) * 0.3f;
                helper_drone.y += (target_y - helper_drone.y) * 0.25f;

                // Dedicated bounds clamping: strictly in front of player & within viewport
                if (helper_drone.x < PLAYER_X + 8.0f) helper_drone.x = PLAYER_X + 8.0f;
                if (helper_drone.y < 20.0f) helper_drone.y = 20.0f;
                if (helper_drone.y > 138.0f) helper_drone.y = 138.0f;

                helper_drone.shoot_cooldown--;
                if (helper_drone.shoot_cooldown <= 0) {
                    helper_drone.shoot_cooldown = 18;
                    for (int b = 0; b < MAX_BULLETS; b++) {
                        if (!bullets[b].active) {
                            bullets[b].active = true;
                            bullets[b].x = helper_drone.x + 6;
                            bullets[b].y = helper_drone.y + 2;
                            play_tone(950, 2);
                            break;
                        }
                    }
                }
            }
        }

        // Singularity Black Hole Vortex (10s = 300 frames)
        if (black_hole.active) {
            black_hole.timer--;
            black_hole.pulse++;
            if (black_hole.timer <= 0) {
                black_hole.active = false;
                play_tone(300, 4);
            } else {
                // Gravitational Pull & Destruction on all active enemies toward (64, 85)
                for (int i = 0; i < MAX_ENEMIES; i++) {
                    if (enemies[i].active) {
                        float dx = black_hole.x - enemies[i].x;
                        float dy = black_hole.y - enemies[i].y;
                        float dist = sqrtf(dx * dx + dy * dy);
                        if (dist < 10.0f) {
                            // Event Horizon Annihilation!
                            spawn_explosion((int)enemies[i].x, (int)enemies[i].y);
                            enemies[i].x = SCREEN_WIDTH + rand() % 30;
                            enemies[i].base_y = 20 + rand() % (SCREEN_HEIGHT - 40);
                            enemies[i].y = enemies[i].base_y;
                            score += 2;
                            combo_count++;
                            play_tone(1200, 2);
                        } else {
                            enemies[i].x += (dx / (dist + 0.1f)) * 2.2f;
                            enemies[i].y += (dy / (dist + 0.1f)) * 2.2f;
                        }
                    }
                }
            }
        }

        static int bgm_frame = 0;
        bgm_frame++;
        update_bgm(bgm_frame);

        // Hyper Overload (Auto rapid fire)
        if (overload_active) {
            overload_timer--;
            if (overload_timer <= 0) {
                overload_active = false;
                play_tone(300, 4);
            } else if (bgm_frame % 3 == 0) {
                fire_bullet();
            }
        }
        if (player.y + player.height > SCREEN_HEIGHT) {
            // Drop player to center and deduct life
            spawn_explosion(PLAYER_X, (int)player.y);
            trigger_screenshake(4, 8); // Rumble screen
            lives--;
            
            // Set HUD to redraw
            draw_shake_offset_x = 0;
            draw_shake_offset_y = 0;
            draw_lives(lives);
            draw_shake_offset_x = o_shake_x;
            draw_shake_offset_y = o_shake_y;

            if (lives <= 0) {
                trigger_gameover();
                continue;
            } else {
                player.y = SCREEN_HEIGHT / 2.0f;
                player.velocity = 0;
            }
        }

        // Spawn engine thruster flame plume at player rear (uses old shake for drawing, matches erase in next frame)
        spawn_thruster_plume(PLAYER_X - 1, (int)player.y);

        // Update Bullets (Any boundary erases use o_shake)
        for (int i = 0; i < MAX_BULLETS; i++) {
            if (bullets[i].active) {
                old_bullet_x[i] = bullets[i].x;
                old_bullet_y[i] = bullets[i].y;
                bullets[i].x += 4;
                if (bullets[i].x >= SCREEN_WIDTH) {
                    bullets[i].active = false;
                }
            }
        }

        // Update Boss Mechanics
        int old_boss_x = (int)boss_x;
        int old_boss_y = (int)boss_y;
        if (boss_active) {
            // Universal entrance: all 10 boss types advance from off-screen (138) to viewport entry position
            float target_entry_x = (current_boss_type == BOSS_VIPER) ? 80.0f : 102.0f;
            if (boss_x > target_entry_x) {
                boss_x -= 0.6f;
            } else {
                boss_x = target_entry_x;
                boss_y += boss_vy;
                if (boss_y < 22.0f || boss_y > 126.0f) {
                    boss_vy = -boss_vy;
                    if (boss_y < 22.0f) boss_y = 22.0f;
                    if (boss_y > 126.0f) boss_y = 126.0f;
                }
            }

            boss_shoot_timer++;

            if (current_boss_type == BOSS_MOTHERSHIP) {
                // Mothership weapon: Single Sniper Bullet
                if (boss_shoot_timer >= 65) {
                    boss_shoot_timer = 0;
                    for (int k = 0; k < MAX_BOSS_BULLETS; k++) {
                        if (!boss_bullets[k].active) {
                            boss_bullets[k].active = true;
                            boss_bullets[k].x = boss_x;
                            boss_bullets[k].y = boss_y + 6;
                            boss_bullets[k].vx = -2.5f;
                            boss_bullets[k].vy = 0.0f;
                            play_tone(330, 3);
                            break;
                        }
                    }
                }
            } else {
                // Boss attack logic for all 10 boss types
                if (current_boss_type == BOSS_CHRONO) {
                    if (boss_shoot_timer % 40 == 0) {
                        boss_y = 24.0f + (rand() % (SCREEN_HEIGHT - 56));
                        play_tone(1200, 3);
                    }
                }

                if (boss_shoot_timer % 55 == 0) {
                    int launched = 0;
                    for (int k = 0; k < MAX_BOSS_BULLETS; k++) {
                        if (!boss_bullets[k].active) {
                            boss_bullets[k].active = true;
                            boss_bullets[k].x = boss_x;
                            boss_bullets[k].y = boss_y + 6;

                            if (current_boss_type == BOSS_ASTEROID) {
                                boss_bullets[k].vx = -2.2f;
                                boss_bullets[k].vy = (launched == 0) ? -0.8f : 0.8f;
                                launched++;
                                if (launched >= 2) break;
                            } else if (current_boss_type == BOSS_DRAGON) {
                                boss_bullets[k].vx = -2.8f;
                                boss_bullets[k].vy = (launched == 0) ? -1.2f : ((launched == 1) ? 0.0f : 1.2f);
                                launched++;
                                if (launched >= 3) break;
                            } else if (current_boss_type == BOSS_NEBULA) {
                                boss_bullets[k].vx = -2.0f;
                                boss_bullets[k].vy = sinf(boss_shoot_timer * 0.1f) * 1.5f;
                                launched++;
                                if (launched >= 1) break;
                            } else if (current_boss_type == BOSS_OMEGA) {
                                float angles[4] = {-1.0f, -0.4f, 0.4f, 1.0f};
                                boss_bullets[k].vx = -2.5f;
                                boss_bullets[k].vy = angles[launched];
                                launched++;
                                if (launched >= 4) break;
                            } else {
                                boss_bullets[k].vx = (launched == 1) ? -2.4f : -2.1f;
                                boss_bullets[k].vy = (launched == 0) ? -0.5f : ((launched == 1) ? 0.0f : 0.5f);
                                launched++;
                                if (launched >= 3) break;
                            }
                        }
                    }
                    if (launched > 0) play_tone(440, 4);
                }

                // Dreadnought & Omega drone spawns
                if ((current_boss_type == BOSS_DREADNOUGHT || current_boss_type == BOSS_OMEGA) && boss_shoot_timer >= 130) {
                    boss_shoot_timer = 0;
                    for (int i = 0; i < MAX_ENEMIES; i++) {
                        if (!enemies[i].active) {
                            enemies[i].active = true;
                            enemies[i].x = boss_x - 4;
                            enemies[i].base_y = boss_y + 4;
                            enemies[i].y = enemies[i].base_y;
                            enemies[i].speed = 1.0f + (score * 0.005f);
                            enemies[i].type = (rand() % 2 == 0) ? ENEMY_SCOUT : ENEMY_CHARGER;
                            enemies[i].dived = false;
                            play_tone(784, 3);
                            break;
                        }
                    }
                }
            }
        }

        // Update Boss Bullets
        for (int k = 0; k < MAX_BOSS_BULLETS; k++) {
            if (boss_bullets[k].active) {
                old_boss_bullet_x[k] = (int)boss_bullets[k].x;
                old_boss_bullet_y[k] = (int)boss_bullets[k].y;
                
                // Move bullet
                boss_bullets[k].x += boss_bullets[k].vx;
                boss_bullets[k].y += boss_bullets[k].vy;
                
                if (boss_bullets[k].x < -5 || boss_bullets[k].y < 0 || boss_bullets[k].y > SCREEN_HEIGHT) {
                    boss_bullets[k].active = false;
                }
            }
        }

        // Update Enemy Bullets
        for (int k = 0; k < MAX_ENEMY_BULLETS; k++) {
            if (enemy_bullets[k].active) {
                old_enemy_bullet_x[k] = (int)enemy_bullets[k].x;
                old_enemy_bullet_y[k] = (int)enemy_bullets[k].y;
                
                enemy_bullets[k].x -= 2.8f; // Fly left
                
                if (enemy_bullets[k].x < -5) {
                    enemy_bullets[k].active = false;
                }
            }
        }

        // Update Enemies (Only if boss is not active)
        if (!boss_active) {
            // Spawn delay logic for smart bomb screen clear
            if (enemy_spawn_delay_timer > 0) {
                enemy_spawn_delay_timer--;
                if (enemy_spawn_delay_timer == 0) {
                    // Reactivate standard enemies on the far right
                    for (int i = 0; i < MAX_ENEMIES; i++) {
                        enemies[i].active = true;
                        enemies[i].x = SCREEN_WIDTH + 20 + (i * 45);
                        enemies[i].base_y = 24 + rand() % (SCREEN_HEIGHT - 48);
                        enemies[i].y = enemies[i].base_y;
                        enemies[i].speed = 0.8f + (score * 0.005f) + (rand() % 80) / 100.0f;
                        enemies[i].dived = false;
                        enemies[i].shoot_cooldown = 40 + rand() % 80;
                        enemies[i].type = get_progressive_enemy_type(score);
                        if (enemies[i].type == ENEMY_CHARGER) {
                            enemies[i].speed *= 1.4f;
                        }
                    }
                }
            }

            for (int i = 0; i < MAX_ENEMIES; i++) {
                if (enemies[i].active) {
                    old_enemy_x[i] = (int)enemies[i].x;
                    old_enemy_y[i] = (int)enemies[i].y;
                    enemies[i].x -= enemies[i].speed;

                    // Sinusoidal movement path for Bomber type enemies
                    if (enemies[i].type == ENEMY_BOMBER) {
                        enemies[i].y = enemies[i].base_y + sinf(enemies[i].x * 0.08f) * 12.0f;
                        if (enemies[i].y < 19.0f) enemies[i].y = 19.0f;
                        if (enemies[i].y > SCREEN_HEIGHT - 10) enemies[i].y = SCREEN_HEIGHT - 10;
                    }
                    
                    // Homing dive path for Diver type enemies when within proximity
                    if (enemies[i].type == ENEMY_DIVER) {
                        if (!enemies[i].dived && enemies[i].x < PLAYER_X + 65 && enemies[i].x > PLAYER_X) {
                            enemies[i].dived = true;
                            enemies[i].base_y = player.y; // Lock to the player's Y coordinate at initiation
                        }
                        if (enemies[i].dived) {
                            enemies[i].y += (enemies[i].base_y - enemies[i].y) * 0.1f;
                            if (enemies[i].y < 19.0f) enemies[i].y = 19.0f;
                            if (enemies[i].y > SCREEN_HEIGHT - 10) enemies[i].y = SCREEN_HEIGHT - 10;
                        }
                    }

                    // Shooter enemy firing mechanics
                    if (enemies[i].type == ENEMY_SHOOTER) {
                        enemies[i].shoot_cooldown--;
                        if (enemies[i].shoot_cooldown <= 0) {
                            enemies[i].shoot_cooldown = 70 + rand() % 50;
                            for (int k = 0; k < MAX_ENEMY_BULLETS; k++) {
                                if (!enemy_bullets[k].active) {
                                    enemy_bullets[k].active = true;
                                    enemy_bullets[k].x = enemies[i].x - 3;
                                    enemy_bullets[k].y = enemies[i].y + 3;
                                    play_tone(400, 2);
                                    break;
                                }
                            }
                        }
                    }

                    // Respawn enemy if it exits screen
                    if (enemies[i].x < -10) {
                        enemies[i].x = SCREEN_WIDTH + rand() % 30;
                        enemies[i].base_y = 20 + rand() % (SCREEN_HEIGHT - 40);
                        enemies[i].y = enemies[i].base_y;
                        enemies[i].dived = false;
                        enemies[i].shoot_cooldown = 40 + rand() % 80;
                        enemies[i].type = get_progressive_enemy_type(score);
                        
                        // Level Progression: enemy base speed increases linearly with score!
                        enemies[i].speed = 0.8f + (score * 0.005f) + (rand() % 80) / 100.0f;
                        
                        if (enemies[i].type == ENEMY_CHARGER) {
                            enemies[i].speed *= 1.4f;
                        }
                    }
                }
            }
        }

        // Update Power Up Item (Erase uses o_shake)
        if (power_up.active) {
            power_up.x -= 1.0f;
            if (power_up.x < -10) {
                power_up.active = false;
            }
        }

        // Collisions: Bullet vs Enemy (Erases use o_shake, only active when boss is dead)
        if (!boss_active) {
            for (int b = 0; b < MAX_BULLETS; b++) {
                if (!bullets[b].active) continue;
                for (int e = 0; e < MAX_ENEMIES; e++) {
                    if (!enemies[e].active) continue;

                    // Simple AABB Collision check
                    if (bullets[b].x >= enemies[e].x && bullets[b].x <= enemies[e].x + 8 &&
                        bullets[b].y >= enemies[e].y && bullets[b].y <= enemies[e].y + 8) {
                        
                        // Hit!
                        bullets[b].active = false;
                        erase_sprite(old_bullet_x[b], old_bullet_y[b], 8, 4);

                        if (enemies[e].hp > 1) {
                            enemies[e].hp--;
                            play_tone(440, 2);
                            uint16_t spark_col = (enemies[e].type == ENEMY_COMMANDER) ? COLOR_RED : C_CYAN;
                            spawn_particle_single(enemies[e].x, enemies[e].y, -1.0f, 0.5f, spark_col);
                            spawn_particle_single(enemies[e].x, enemies[e].y, -1.0f, -0.5f, spark_col);
                            break;
                        }

                        // Trigger Particle Blast & camera screenshake
                        spawn_explosion((int)enemies[e].x, (int)enemies[e].y);
                        trigger_screenshake(2, 4);

                        // Chance to drop a powerup at enemy's location (25% chance)
                        if (!power_up.active && (rand() % 4 == 0)) {
                            power_up.active = true;
                            power_up.x = enemies[e].x;
                            power_up.y = enemies[e].y;
                            power_up.type = get_progressive_powerup_type(score);
                            power_up.duration_frames = 0;

                            play_tone(1046, 3);
                            spawn_explosion((int)power_up.x, (int)power_up.y);
                        }

                        erase_sprite((int)enemies[e].x, (int)enemies[e].y, 8, 8); // Clear enemy sprite
                        enemies[e].x = SCREEN_WIDTH + rand() % 30;
                        enemies[e].base_y = 24 + rand() % (SCREEN_HEIGHT - 48);
                        enemies[e].y = enemies[e].base_y;
                        
                        // Level Progression Speed Scaling
                        enemies[e].speed = 0.8f + (score * 0.005f) + (rand() % 80) / 100.0f;
                        
                        if (enemies[e].type == ENEMY_CHARGER || enemies[e].type == ENEMY_SWARMER) {
                            enemies[e].speed *= 1.4f;
                        }
                        enemies[e].hp = (enemies[e].type == ENEMY_COMMANDER) ? 3 : ((enemies[e].type == ENEMY_SHIELDED) ? 2 : 1);

                        // Combo streak & floating text popup
                        if (combo_timer > 0) {
                            combo_count++;
                        } else {
                            combo_count = 1;
                        }
                        combo_timer = 45; // 1.5s combo window

                        int add_score = combo_count;
                        score += add_score;

                        char pop_buf[16];
                        if (combo_count == 1) {
                            snprintf(pop_buf, sizeof(pop_buf), "+10");
                        } else {
                            snprintf(pop_buf, sizeof(pop_buf), "+%d x%d!", add_score * 10, combo_count);
                        }
                        uint16_t pop_col = (combo_count >= 3) ? COLOR_YELLOW : ((combo_count == 2) ? C_CYAN : COLOR_WHITE);
                        spawn_floating_text(enemies[e].x, enemies[e].y - 4, pop_buf, pop_col);

                        // New High Score Announcement
                        if (score > high_score && high_score > 0 && !new_best_announced) {
                            new_best_announced = true;
                            toast_timer = 45;
                            toast_text = "NEW HIGH SCORE!";
                            toast_color = COLOR_YELLOW;
                            play_tone(1046, 3);
                            play_tone(1318, 5);
                        }
                        
                        // Redraw static HUD elements (un-shaked)
                        draw_shake_offset_x = 0;
                        draw_shake_offset_y = 0;
                        draw_score(score);
                        draw_shake_offset_x = o_shake_x;
                        draw_shake_offset_y = o_shake_y;
                        break;
                    }
                }
            }
        }

        // Collisions: Bullet vs Boss (Erases use o_shake)
        if (boss_active) {
            for (int b = 0; b < MAX_BULLETS; b++) {
                if (!bullets[b].active) continue;

                // Boss is 16x16, at boss_x, boss_y
                if (bullets[b].x >= boss_x && bullets[b].x <= boss_x + 16 &&
                    bullets[b].y >= boss_y && bullets[b].y <= boss_y + 16) {
                    
                    bullets[b].active = false;
                    erase_sprite(old_bullet_x[b], old_bullet_y[b], 8, 4);

                    boss_hp--;
                    spawn_explosion(bullets[b].x, bullets[b].y);
                    trigger_screenshake(3, 5);
                    play_tone(660, 2); // Hit chime

                    if (boss_hp <= 0) {
                        boss_active = false;
                        
                        // Clear boss health bar panel from HUD
                        draw_shake_offset_x = 0;
                        draw_shake_offset_y = 0;
                        draw_rect(0, 148, 128, 12, COLOR_BLACK);
                        draw_shake_offset_x = shake_offset_x;
                        draw_shake_offset_y = shake_offset_y;

                        // Erase boss completely using o_shake
                        erase_sprite(old_boss_x - 1, old_boss_y - 1, 18, 18);
                        for (int k = 0; k < MAX_BOSS_BULLETS; k++) {
                            if (boss_bullets[k].active) {
                                erase_sprite(old_boss_bullet_x[k], old_boss_bullet_y[k], 4, 4);
                                boss_bullets[k].active = false;
                            }
                        }

                        // Giant explosion effects!
                        for (int k = 0; k < 4; k++) {
                            spawn_explosion((int)boss_x + rand() % 16, (int)boss_y + rand() % 16);
                        }
                        trigger_screenshake(8, 15);
                        play_tone(110, 15); // Long rumble bass explosion

                        score += 5; // Defeat bonus
                        
                        // Boss power-up drop: 2 to 5 minute durations (No forever/permanent drops)
                        if (!power_up.active) {
                            power_up.active = true;
                            power_up.x = boss_x + 4;
                            power_up.y = boss_y + 4;
                            
                            if (current_boss_type == BOSS_MOTHERSHIP || current_boss_type == BOSS_DREADNOUGHT) {
                                power_up.duration_frames = 3600; // 2 Minutes
                                int roll = rand() % 4;
                                if (roll == 0)      power_up.type = POWERUP_SHIELD;
                                else if (roll == 1) power_up.type = POWERUP_DOUBLE;
                                else if (roll == 2) power_up.type = POWERUP_BOMB;
                                else                power_up.type = POWERUP_LIFE;
                            } else if (current_boss_type == BOSS_VIPER || current_boss_type == BOSS_PHANTOM || current_boss_type == BOSS_TITAN) {
                                power_up.duration_frames = 6300; // 3.5 Minutes
                                power_up.type = (boss_encounter_count % 2 == 1) ? POWERUP_SHIELD : POWERUP_DOUBLE;
                            } else {
                                power_up.duration_frames = 9000; // 5 Minutes
                                int roll = boss_encounter_count % 3;
                                power_up.type = (roll == 0) ? POWERUP_SPREAD : ((roll == 1) ? POWERUP_DOUBLE : POWERUP_SHIELD);
                            }
                            play_tone(1046, 4);
                        }

                        // Redraw HUD
                        draw_shake_offset_x = 0;
                        draw_shake_offset_y = 0;
                        draw_score(score);
                        draw_shake_offset_x = o_shake_x;
                        draw_shake_offset_y = o_shake_y;

                        // Re-enable normal enemies with randomized types
                        for (int i = 0; i < MAX_ENEMIES; i++) {
                            enemies[i].active = true;
                            enemies[i].x = SCREEN_WIDTH + 30 + (i * 55);
                            enemies[i].base_y = 24 + rand() % (SCREEN_HEIGHT - 48);
                            enemies[i].y = enemies[i].base_y;
                            enemies[i].speed = 0.8f + (score * 0.005f) + (rand() % 80) / 100.0f;
                            enemies[i].dived = false;
                            enemies[i].shoot_cooldown = 40 + rand() % 80;
                            enemies[i].type = get_progressive_enemy_type(score);
                            
                            if (enemies[i].type == ENEMY_CHARGER) {
                                enemies[i].speed *= 1.4f;
                            }
                        }
                    }
                    break;
                }
            }
        }

        // Collisions: Player vs Power-up (Erase uses o_shake)
        if (power_up.active) {
            if (PLAYER_X + player.width >= power_up.x && PLAYER_X <= power_up.x + 8 &&
                (int)player.y + player.height >= power_up.y && (int)player.y <= power_up.y + 8) {
                
                PowerUpType collected_type = power_up.type;
                int item_dur = power_up.duration_frames;
                power_up.active = false;
                erase_sprite((int)power_up.x - 1, power_up.y - 1, 10, 10);
                
                // Play retro high-pitched chime sound & spark particles
                play_tone(1200, 4);
                spawn_explosion(PLAYER_X + 4, (int)player.y + 4);

                toast_timer = 40; // Reset toast timer so new pickup is visible

                int dur = (item_dur > 0) ? item_dur : get_powerup_duration(score);

                if (collected_type == POWERUP_SHIELD) {
                    shield_active = true;
                    shield_timer = dur;
                    if (dur >= 9000)      toast_text = "+ SHIELD (5m)!";
                    else if (dur >= 6300) toast_text = "+ SHIELD (3.5m)!";
                    else if (dur >= 3600) toast_text = "+ SHIELD (2m)!";
                    else if (dur >= 1800) toast_text = "+ SHIELD (60s)!";
                    else                  toast_text = "+ SHIELD (30s)!";
                    toast_color = C_CYAN;
                    if (!tutorial_shield_done) {
                        tutorial_shield_done = true;
                        show_tutorial_overlay(POWERUP_SHIELD);
                    }
                } else if (collected_type == POWERUP_DOUBLE) {
                    double_shot_active = true;
                    double_shot_timer = dur;
                    if (dur >= 9000)      toast_text = "+ DUAL GUN (5m)!";
                    else if (dur >= 6300) toast_text = "+ DUAL GUN (3m)!";
                    else if (dur >= 3600) toast_text = "+ DUAL GUN (2m)!";
                    else if (dur >= 1800) toast_text = "+ DUAL GUN (60s)!";
                    else                  toast_text = "+ DUAL GUN (30s)!";
                    toast_color = COLOR_YELLOW;
                    if (!tutorial_double_done) {
                        tutorial_double_done = true;
                        show_tutorial_overlay(POWERUP_DOUBLE);
                    }
                } else if (collected_type == POWERUP_SPREAD) {
                    spread_shot_active = true;
                    spread_shot_timer = dur;
                    if (dur >= 9000)      toast_text = "+ SPREAD (5m)!";
                    else if (dur >= 6300) toast_text = "+ SPREAD (3m)!";
                    else if (dur >= 3600) toast_text = "+ SPREAD (2m)!";
                    else if (dur >= 1800) toast_text = "+ SPREAD (60s)!";
                    else                  toast_text = "+ SPREAD (30s)!";
                    toast_color = 0xFD20; // Orange
                    if (!tutorial_spread_done) {
                        tutorial_spread_done = true;
                        show_tutorial_overlay(POWERUP_SPREAD);
                    }
                } else if (collected_type == POWERUP_DRONE) {
                    helper_drone.active = true;
                    helper_drone.timer = 450; // 15 seconds
                    helper_drone.x = PLAYER_X;
                    helper_drone.y = player.y;
                    toast_text = "+ HELPER DRONE!";
                    toast_color = C_CYAN;
                    if (!tutorial_drone_done) {
                        tutorial_drone_done = true;
                        show_tutorial_overlay(POWERUP_DRONE);
                    }
                } else if (collected_type == POWERUP_BLACKHOLE) {
                    black_hole.active = true;
                    black_hole.timer = 300; // 10 seconds
                    black_hole.x = 64.0f;
                    black_hole.y = 85.0f;
                    toast_text = "+ BLACK HOLE!";
                    toast_color = COLOR_MAGENTA;
                    if (!tutorial_blackhole_done) {
                        tutorial_blackhole_done = true;
                        show_tutorial_overlay(POWERUP_BLACKHOLE);
                    }
                } else if (collected_type == POWERUP_OVERLOAD) {
                    overload_active = true;
                    overload_timer = 240; // 8 seconds
                    toast_text = "+ HYPER OVERLOAD";
                    toast_color = COLOR_RED;
                    if (!tutorial_overload_done) {
                        tutorial_overload_done = true;
                        show_tutorial_overlay(POWERUP_OVERLOAD);
                    }
                } else if (collected_type == POWERUP_BOMB) {
                    bomb_count++; // Stacks!
                    toast_text = "+ SMART BOMB!";
                    toast_color = COLOR_MAGENTA;
                    
                    // Update HUD bomb counter
                    draw_shake_offset_x = 0;
                    draw_shake_offset_y = 0;
                    draw_score(score);
                    draw_shake_offset_x = shake_offset_x;
                    draw_shake_offset_y = shake_offset_y;
                } else if (collected_type == POWERUP_LIFE) {
                    toast_text = "+ EXTRA LIFE!";
                    toast_color = COLOR_GREEN;
                    if (lives < 5) {
                        lives++;
                        draw_shake_offset_x = 0;
                        draw_shake_offset_y = 0;
                        draw_lives(lives);
                        draw_shake_offset_x = shake_offset_x;
                        draw_shake_offset_y = shake_offset_y;
                    }
                }
            }
        }

        // Collisions: Player vs Boss Bullets
        if (boss_active) {
            for (int k = 0; k < MAX_BOSS_BULLETS; k++) {
                if (boss_bullets[k].active) {
                    if (PLAYER_X + player.width >= boss_bullets[k].x && PLAYER_X <= boss_bullets[k].x + 4 &&
                        (int)player.y + player.height >= boss_bullets[k].y && (int)player.y <= boss_bullets[k].y + 4) {
                        
                        boss_bullets[k].active = false;
                        erase_sprite((int)boss_bullets[k].x, (int)boss_bullets[k].y, 4, 4);
                        trigger_screenshake(5, 10);
                        spawn_explosion(PLAYER_X, (int)player.y);

                        if (shield_active) {
                            shield_active = false;
                            play_tone(440, 4);
                            // Wipe full shield ring footprint (2px ring + corner dots = need 3px margin)
                            draw_rect(PLAYER_X - 3, old_player_y - 3, player.width + 6, player.height + 6, COLOR_BLACK);
                        } else {
                            play_tone(220, 6);
                            lives--;
                            
                            // Update static HUD
                            draw_shake_offset_x = 0;
                            draw_shake_offset_y = 0;
                            draw_lives(lives);
                            draw_shake_offset_x = o_shake_x;
                            draw_shake_offset_y = o_shake_y;

                            if (lives <= 0) {
                                trigger_gameover();
                                break;
                            }
                        }
                    }
                }
            }
        }

        // Collisions: Player vs Enemy Bullets
        for (int k = 0; k < MAX_ENEMY_BULLETS; k++) {
            if (enemy_bullets[k].active) {
                if (PLAYER_X + player.width >= enemy_bullets[k].x && PLAYER_X <= enemy_bullets[k].x + 3 &&
                    (int)player.y + player.height >= enemy_bullets[k].y && (int)player.y <= enemy_bullets[k].y + 3) {
                    
                    enemy_bullets[k].active = false;
                    erase_sprite((int)enemy_bullets[k].x, (int)enemy_bullets[k].y, 3, 3);
                    trigger_screenshake(4, 8);
                    spawn_explosion(PLAYER_X, (int)player.y);

                    if (shield_active) {
                        shield_active = false;
                        play_tone(440, 4);
                        // Wipe full shield ring footprint (2px ring + corner dots = need 3px margin)
                        draw_rect(PLAYER_X - 3, old_player_y - 3, player.width + 6, player.height + 6, COLOR_BLACK);
                    } else {
                        play_tone(220, 6);
                        lives--;
                        
                        // Update static HUD
                        draw_shake_offset_x = 0;
                        draw_shake_offset_y = 0;
                        draw_lives(lives);
                        draw_shake_offset_x = o_shake_x;
                        draw_shake_offset_y = o_shake_y;

                        if (lives <= 0) {
                            trigger_gameover();
                            break;
                        }
                    }
                }
            }
        }

        // Collisions: Player vs Enemy (Erases use o_shake, only active when boss is dead)
        if (!boss_active) {
            for (int e = 0; e < MAX_ENEMIES; e++) {
                if (!enemies[e].active) continue;

                if (PLAYER_X + player.width >= enemies[e].x && PLAYER_X <= enemies[e].x + 8 &&
                    (int)player.y + player.height >= enemies[e].y && (int)player.y <= enemies[e].y + 8) {
                    
                    spawn_explosion((int)enemies[e].x, (int)enemies[e].y);
                    erase_sprite((int)enemies[e].x, (int)enemies[e].y, 8, 8);
                    
                    // Respawn enemy immediately
                    enemies[e].x = SCREEN_WIDTH + rand() % 30;
                    enemies[e].base_y = 20 + rand() % (SCREEN_HEIGHT - 40);
                    enemies[e].y = enemies[e].base_y;
                    enemies[e].dived = false;
                    enemies[e].shoot_cooldown = 40 + rand() % 80;
                    enemies[e].type = get_progressive_enemy_type(score);
                    
                    // Level Progression Speed Scaling
                    enemies[e].speed = 0.8f + (score * 0.005f) + (rand() % 80) / 100.0f;
                    
                    if (enemies[e].type == ENEMY_CHARGER) {
                        enemies[e].speed *= 1.4f;
                    }

                    // Shake camera aggressively on impact
                    trigger_screenshake(5, 10);

                    if (shield_active) {
                        // Shield absorbs hit
                        shield_active = false;
                        play_tone(440, 4);
                        // Wipe full shield ring footprint (2px ring + corner dots = need 3px margin)
                        draw_rect(PLAYER_X - 3, old_player_y - 3, player.width + 6, player.height + 6, COLOR_BLACK);
                    } else {
                        // Lose a life (play low sad buzzer tone)
                        play_tone(220, 6);
                        lives--;
                        
                        // Update static HUD
                        draw_shake_offset_x = 0;
                        draw_shake_offset_y = 0;
                        draw_lives(lives);
                        draw_shake_offset_x = o_shake_x;
                        draw_shake_offset_y = o_shake_y;

                        if (lives <= 0) {
                            trigger_gameover();
                            break;
                        }
                    }
                }
            }
        }

        if (current_state == STATE_GAMEOVER) continue;

        // --- Render Frame: Erase Stage (Uses old shake offsets) ---
        draw_shake_offset_x = o_shake_x;
        draw_shake_offset_y = o_shake_y;

        // 1. Erase Player (extra margin covers 2px shield ring + 2px charge bar)
        draw_rect(PLAYER_X - 3, old_player_y - 3, player.width + 6, player.height + 8, COLOR_BLACK);
        
        // State-machine auto-cleaning tracking flags
        static bool bullet_was_drawn[MAX_BULLETS] = {false};
        static bool enemy_was_drawn[MAX_ENEMIES] = {false};
        static bool enemy_bullet_was_drawn[MAX_ENEMY_BULLETS] = {false};
        static bool boss_bullet_was_drawn[MAX_BOSS_BULLETS] = {false};
        static bool powerup_was_drawn = false;
        static bool boss_was_drawn = false;
        static int old_powerup_x = 0, old_powerup_y = 0;

        // 2. Erase Player Bullets (State-machine auto-cleaning prevents ghosting)
        for (int i = 0; i < MAX_BULLETS; i++) {
            if (bullet_was_drawn[i]) {
                erase_sprite(old_bullet_x[i] - 1, old_bullet_y[i] - 1, 10, 6);
                bullet_was_drawn[i] = false;
            }
        }

        // 3. Erase Enemies or Boss (State-machine auto-cleaning prevents ghosting)
        for (int i = 0; i < MAX_ENEMIES; i++) {
            if (enemy_was_drawn[i]) {
                erase_sprite(old_enemy_x[i] - 1, old_enemy_y[i] - 1, 10, 10);
                enemy_was_drawn[i] = false;
            }
        }

        if (boss_was_drawn) {
            if (current_boss_type == BOSS_VIPER) {
                erase_sprite(old_boss_x - 2, old_boss_y - 8, 46, 30);
            } else {
                erase_sprite(old_boss_x - 2, old_boss_y - 2, 20, 20);
            }
            boss_was_drawn = false;
        }

        for (int k = 0; k < MAX_BOSS_BULLETS; k++) {
            if (boss_bullet_was_drawn[k]) {
                erase_sprite(old_boss_bullet_x[k] - 1, old_boss_bullet_y[k] - 1, 8, 6);
                boss_bullet_was_drawn[k] = false;
            }
        }

        // 4. Erase Power-up Item (State-machine auto-cleaning prevents ghosting)
        if (powerup_was_drawn) {
            erase_sprite(old_powerup_x - 1, old_powerup_y - 1, 11, 11);
            powerup_was_drawn = false;
        }

        // Erase pickup toast banner area
        static bool toast_was_drawn = false;
        if (toast_timer > 0) {
            draw_rect(0, 20, 128, 14, COLOR_BLACK);
        } else if (toast_was_drawn) {
            draw_rect(0, 20, 128, 14, COLOR_BLACK);
            toast_was_drawn = false;
        }

        // 4b. Erase Enemy Bullets (State-machine auto-cleaning prevents ghosting)
        for (int k = 0; k < MAX_ENEMY_BULLETS; k++) {
            if (enemy_bullet_was_drawn[k]) {
                erase_sprite(old_enemy_bullet_x[k] - 1, old_enemy_bullet_y[k] - 1, 6, 5);
                enemy_bullet_was_drawn[k] = false;
            }
        }

        // 5. Erase Radial Blast Shockwave — clear all 3 rings drawn last frame
        if (radial_blast_drawn_radius > 0) {
            int r  = radial_blast_drawn_radius;
            int mr = (r * 4) / 5;       // mid ring ratio matches draw
            int ir = (r * 11) / 20;     // inner ring ratio matches draw
            // Helper: erase one rectangular ring
            auto erase_ring = [&](int rad) {
                if (rad < 1) return;
                draw_rect(radial_blast_x - rad, radial_blast_y - rad, rad * 2 + 2, 2, COLOR_BLACK);
                draw_rect(radial_blast_x - rad, radial_blast_y + rad, rad * 2 + 2, 2, COLOR_BLACK);
                draw_rect(radial_blast_x - rad, radial_blast_y - rad, 2, rad * 2 + 2, COLOR_BLACK);
                draw_rect(radial_blast_x + rad, radial_blast_y - rad, 2, rad * 2 + 2, COLOR_BLACK);
            };
            erase_ring(r);
            erase_ring(mr);
            erase_ring(ir);
            radial_blast_drawn_radius = 0;
        }

        // --- Update Screenshake & Sound Timers to calculate new offset ---
        update_screenshake();
        update_audio();
        if (radial_blast_timer > 0) {
            radial_blast_timer--;
        }

        // Starfield, particles, and floating text popups bypass global draw offset
        update_and_draw_starfield(o_shake_x, o_shake_y, shake_offset_x, shake_offset_y);
        update_and_draw_particles(o_shake_x, o_shake_y, shake_offset_x, shake_offset_y);
        update_and_draw_floating_text(o_shake_x, o_shake_y, shake_offset_x, shake_offset_y);

        // --- Render Frame: Draw Stage (Uses new shake offsets) ---
        draw_shake_offset_x = shake_offset_x;
        draw_shake_offset_y = shake_offset_y;

        // 1. Draw Player Ship
        draw_sprite(PLAYER_X, (int)player.y, player.width, player.height, sprite_player);
        
        // Animated shield aura — pulses between bright cyan and dim blue
        if (shield_active) {
            // Use a simple counter derived from frame timing for pulse
            static int shield_pulse = 0;
            shield_pulse++;
            uint16_t s_col = (shield_pulse / 4 % 2 == 0) ? C_CYAN : 0x034B;
            uint16_t s_corner = (shield_pulse / 4 % 2 == 0) ? 0x03EF : 0x0155;
            // Outer ring
            draw_rect(PLAYER_X - 2, (int)player.y - 2, player.width + 4, 1, s_col);   // Top
            draw_rect(PLAYER_X - 2, (int)player.y + player.height + 1, player.width + 4, 1, s_col); // Bottom
            draw_rect(PLAYER_X - 2, (int)player.y - 1, 1, player.height + 2, s_col);  // Left
            draw_rect(PLAYER_X + player.width + 1, (int)player.y - 1, 1, player.height + 2, s_col); // Right
            // Inner corner dots for rounded feel
            draw_rect(PLAYER_X - 1, (int)player.y - 1, 1, 1, s_corner);
            draw_rect(PLAYER_X + player.width, (int)player.y - 1, 1, 1, s_corner);
            draw_rect(PLAYER_X - 1, (int)player.y + player.height, 1, 1, s_corner);
            draw_rect(PLAYER_X + player.width, (int)player.y + player.height, 1, 1, s_corner);
        }

        // Render Helper Drone Wingman Bot (State-machine auto-cleaning prevents ghosting)
        static int old_drone_x = 0, old_drone_y = 0;
        static bool drone_was_drawn = false;
        if (helper_drone.active) {
            if (drone_was_drawn) {
                erase_sprite(old_drone_x, old_drone_y, 6, 6);
            }
            int dx = (int)helper_drone.x;
            int dy = (int)helper_drone.y;
            for (int r = 0; r < 6; r++) {
                int py = dy + r;
                if (py < 19 || py > 145) continue;
                for (int c = 0; c < 6; c++) {
                    int px = dx + c;
                    if (px < 0 || px >= SCREEN_WIDTH) continue;
                    uint16_t col = sprite_helper_drone[r][c];
                    if (col != C_TRANS) {
                        draw_rect(px, py, 1, 1, col);
                    }
                }
            }
            old_drone_x = dx;
            old_drone_y = dy;
            drone_was_drawn = true;
        } else if (drone_was_drawn) {
            erase_sprite(old_drone_x, old_drone_y, 6, 6);
            drone_was_drawn = false;
        }

        // Render Singularity Black Hole Vortex
        static bool bh_was_drawn = false;
        if (black_hole.active) {
            int bh_x = (int)black_hole.x - 5;
            int bh_y = (int)black_hole.y - 5;
            if (bh_was_drawn) {
                erase_sprite(bh_x, bh_y, 10, 10);
            }
            for (int r = 0; r < 10; r++) {
                for (int c = 0; c < 10; c++) {
                    uint16_t col = sprite_blackhole_vortex[r][c];
                    if (col != C_TRANS) {
                        draw_rect(bh_x + c, bh_y + r, 1, 1, col);
                    }
                }
            }
            bh_was_drawn = true;
        } else if (bh_was_drawn) {
            erase_sprite(59, 80, 10, 10);
            bh_was_drawn = false;
        }

        // Gradient charge bar: green -> yellow -> white, glows when full
        if (bomb_count > 0 && button_hold_frames > 0) {
            int charge_w = (button_hold_frames * 8) / 25;
            if (charge_w > 8) charge_w = 8;
            // Pick color based on charge level
            uint16_t bar_col;
            if      (charge_w >= 8) bar_col = COLOR_WHITE;   // full — white glow
            else if (charge_w >= 5) bar_col = COLOR_YELLOW;  // mid  — yellow
            else                    bar_col = COLOR_GREEN;    // low  — green
            // Draw 2px tall bar for visibility
            draw_rect(PLAYER_X, (int)player.y + player.height + 1, charge_w, 2, bar_col);
            // Clear unfilled portion
            if (charge_w < 8)
                draw_rect(PLAYER_X + charge_w, (int)player.y + player.height + 1, 8 - charge_w, 2, COLOR_BLACK);
        }
        old_player_y = (int)player.y;

        // 2. Draw Laser Bullets
        for (int i = 0; i < MAX_BULLETS; i++) {
            if (bullets[i].active) {
                draw_sprite_bullet(bullets[i].x, bullets[i].y);
                old_bullet_x[i] = bullets[i].x;
                old_bullet_y[i] = bullets[i].y;
                bullet_was_drawn[i] = true;
            }
        }

        // 3. Draw Enemies or Boss
        if (!boss_active) {
            if (boss_was_active) {
                // Instantly clear lingering boss HP bar from HUD when boss is defeated
                draw_shake_offset_x = 0;
                draw_shake_offset_y = 0;
                draw_rect(0, 148, 128, 12, COLOR_BLACK);
                draw_shake_offset_x = shake_offset_x;
                draw_shake_offset_y = shake_offset_y;
            }
            for (int i = 0; i < MAX_ENEMIES; i++) {
                if (enemies[i].active) {
                    const uint16_t (*e_spr)[8] = sprite_enemy;
                    if (enemies[i].type == ENEMY_DIVER)          e_spr = sprite_enemy_diver;
                    else if (enemies[i].type == ENEMY_SHOOTER)   e_spr = sprite_enemy_shooter;
                    else if (enemies[i].type == ENEMY_SHIELDED)  e_spr = sprite_enemy_shielded;
                    else if (enemies[i].type == ENEMY_SWARMER)   e_spr = sprite_enemy_swarmer;
                    else if (enemies[i].type == ENEMY_TURRET)    e_spr = sprite_enemy_turret;
                    else if (enemies[i].type == ENEMY_PHANTOM)   e_spr = sprite_enemy_phantom;
                    else if (enemies[i].type == ENEMY_MINELAYER) e_spr = sprite_enemy_minelayer;
                    else if (enemies[i].type == ENEMY_BEAMER)    e_spr = sprite_enemy_beamer;
                    else if (enemies[i].type == ENEMY_COMMANDER) e_spr = sprite_enemy_commander;

                    if (enemies[i].type == ENEMY_PHANTOM && ((bgm_frame / 6) % 3 == 0)) {
                        // Cloaked stealth state
                    } else {
                        draw_sprite((int)enemies[i].x, (int)enemies[i].y, 8, 8, e_spr);
                        old_enemy_x[i] = (int)enemies[i].x;
                        old_enemy_y[i] = (int)enemies[i].y;
                        enemy_was_drawn[i] = true;
                    }
                }
            }
        } else {
            // Draw Boss by type
            draw_sprite_boss((int)boss_x, (int)boss_y, current_boss_type);
            old_boss_x = (int)boss_x;
            old_boss_y = (int)boss_y;
            boss_was_drawn = true;
            
            // Draw Boss bullets with red core + orange glow trail
            for (int k = 0; k < MAX_BOSS_BULLETS; k++) {
                if (boss_bullets[k].active) {
                    int bx = (int)boss_bullets[k].x;
                    int by = (int)boss_bullets[k].y;
                    // Orange trail (1px behind)
                    draw_rect(bx + 2, by + 1, 3, 2, C_ORANGE);
                    // Bright red core
                    draw_rect(bx,     by,     4, 4, COLOR_RED);
                    // White hot tip
                    draw_rect(bx,     by + 1, 1, 2, COLOR_WHITE);

                    old_boss_bullet_x[k] = bx;
                    old_boss_bullet_y[k] = by;
                    boss_bullet_was_drawn[k] = true;
                }
            }

            // Styled gradient boss HP bar
            draw_shake_offset_x = 0;
            draw_shake_offset_y = 0;
            draw_boss_hp_bar_styled(boss_hp, boss_max_hp);
            draw_shake_offset_x = shake_offset_x;
            draw_shake_offset_y = shake_offset_y;
        }
        boss_was_active = boss_active;

        // 4. Draw Power-up Item with dynamic glowing aura
        if (power_up.active) {
            const uint16_t (*p_sprite)[8] = (power_up.type == POWERUP_SHIELD) ? sprite_powerup_shield : 
                ((power_up.type == POWERUP_DOUBLE) ? sprite_powerup_double : 
                ((power_up.type == POWERUP_BOMB) ? sprite_powerup_bomb : sprite_powerup_life));
            
            draw_sprite_powerup((int)power_up.x, power_up.y, p_sprite);

            // Dynamic glowing 1px pulsing border aura around 8x8 power-up
            static int p_aura_counter = 0;
            p_aura_counter++;
            uint16_t aura_col = COLOR_WHITE;
            if (power_up.type == POWERUP_SHIELD)      aura_col = (p_aura_counter / 4 % 2 == 0) ? C_CYAN : 0x034B;
            else if (power_up.type == POWERUP_DOUBLE) aura_col = (p_aura_counter / 4 % 2 == 0) ? COLOR_YELLOW : 0x8400;
            else if (power_up.type == POWERUP_BOMB)   aura_col = (p_aura_counter / 4 % 2 == 0) ? COLOR_MAGENTA : 0x8010;
            else if (power_up.type == POWERUP_LIFE)   aura_col = (p_aura_counter / 4 % 2 == 0) ? COLOR_GREEN : 0x03E0;

            draw_rect((int)power_up.x - 1, power_up.y - 1, 10, 1, aura_col);
            draw_rect((int)power_up.x - 1, power_up.y + 8, 10, 1, aura_col);
            draw_rect((int)power_up.x - 1, power_up.y - 1, 1, 10, aura_col);
            draw_rect((int)power_up.x + 8, power_up.y - 1, 1, 10, aura_col);

            old_powerup_x = (int)power_up.x;
            old_powerup_y = (int)power_up.y;
            powerup_was_drawn = true;
        }

        // Draw floating pickup toast banner (with auto-erase cleanup on expiration)
        if (toast_timer > 0) {
            toast_timer--;
            int tx = 64 - (strlen(toast_text) * 6) / 2;
            int tw = (strlen(toast_text) * 6) + 8;
            draw_rect(tx - 4, 21, tw, 11, 0x0821);
            draw_rect(tx - 4, 21, tw, 1, toast_color);
            draw_rect(tx - 4, 31, tw, 1, toast_color);
            draw_text(tx, 23, toast_text, toast_color);
            toast_was_drawn = true;
        } else if (toast_was_drawn) {
            draw_rect(0, 20, 128, 14, COLOR_BLACK);
            toast_was_drawn = false;
        }

        // Draw Boss Warning banner (with auto-erase cleanup on off-flashes & expiration)
        static bool warning_was_drawn = false;
        if (boss_warning_timer > 0) {
            boss_warning_timer--;
            draw_shake_offset_x = 0;
            draw_shake_offset_y = 0;
            if ((boss_warning_timer / 6) % 2 == 0) {
                draw_rect(6, 68, 116, 18, 0x8000);
                draw_rect(6, 68, 116, 1, COLOR_RED);
                draw_rect(6, 85, 116, 1, COLOR_RED);
                draw_text(10, 73, "BOSS INCOMING", COLOR_YELLOW);
                warning_was_drawn = true;
            } else if (warning_was_drawn) {
                draw_rect(0, 66, 128, 22, COLOR_BLACK);
                warning_was_drawn = false;
            }
            draw_shake_offset_x = shake_offset_x;
            draw_shake_offset_y = shake_offset_y;
        } else if (warning_was_drawn) {
            draw_shake_offset_x = 0;
            draw_shake_offset_y = 0;
            draw_rect(0, 66, 128, 22, COLOR_BLACK);
            warning_was_drawn = false;
            draw_shake_offset_x = shake_offset_x;
            draw_shake_offset_y = shake_offset_y;
        }

        // Danger Low-Health Red Vignette pulse when at 1 life left (with auto-erase cleanup)
        static bool danger_was_drawn = false;
        if (lives == 1) {
            static int danger_cnt = 0;
            danger_cnt++;
            if (danger_cnt / 12 % 2 == 0) {
                draw_shake_offset_x = 0;
                draw_shake_offset_y = 0;
                draw_rect(0, 18, SCREEN_WIDTH, 1, COLOR_RED);
                draw_rect(0, SCREEN_HEIGHT - 1, SCREEN_WIDTH, 1, COLOR_RED);
                draw_rect(0, 18, 1, SCREEN_HEIGHT - 18, COLOR_RED);
                draw_rect(SCREEN_WIDTH - 1, 18, 1, SCREEN_HEIGHT - 18, COLOR_RED);
                draw_shake_offset_x = shake_offset_x;
                draw_shake_offset_y = shake_offset_y;
                danger_was_drawn = true;
            } else if (danger_was_drawn) {
                draw_shake_offset_x = 0;
                draw_shake_offset_y = 0;
                draw_rect(0, 18, SCREEN_WIDTH, 1, COLOR_BLACK);
                draw_rect(0, SCREEN_HEIGHT - 1, SCREEN_WIDTH, 1, COLOR_BLACK);
                draw_rect(0, 18, 1, SCREEN_HEIGHT - 18, COLOR_BLACK);
                draw_rect(SCREEN_WIDTH - 1, 18, 1, SCREEN_HEIGHT - 18, COLOR_BLACK);
                danger_was_drawn = false;
                draw_shake_offset_x = shake_offset_x;
                draw_shake_offset_y = shake_offset_y;
            }
        } else if (danger_was_drawn) {
            draw_shake_offset_x = 0;
            draw_shake_offset_y = 0;
            draw_rect(0, 18, SCREEN_WIDTH, 1, COLOR_BLACK);
            draw_rect(0, SCREEN_HEIGHT - 1, SCREEN_WIDTH, 1, COLOR_BLACK);
            draw_rect(0, 18, 1, SCREEN_HEIGHT - 18, COLOR_BLACK);
            draw_rect(SCREEN_WIDTH - 1, 18, 1, SCREEN_HEIGHT - 18, COLOR_BLACK);
            danger_was_drawn = false;
            draw_shake_offset_x = shake_offset_x;
            draw_shake_offset_y = shake_offset_y;
        }

        // 4b. Draw Enemy Bullets as plasma shots (magenta core + orange leading edge)
        for (int k = 0; k < MAX_ENEMY_BULLETS; k++) {
            if (enemy_bullets[k].active) {
                int ex = (int)enemy_bullets[k].x;
                int ey = (int)enemy_bullets[k].y;
                // Orange trailing glow
                draw_rect(ex + 1, ey + 1, 3, 1, C_ORANGE);
                // Magenta core
                draw_rect(ex,     ey,     3, 3, COLOR_MAGENTA);
                // Bright leading tip
                draw_rect(ex,     ey + 1, 1, 1, COLOR_WHITE);

                old_enemy_bullet_x[k] = ex;
                old_enemy_bullet_y[k] = ey;
                enemy_bullet_was_drawn[k] = true;
            }
        }

        // 5. Draw Radial Blast Shockwave — concentric rings with color shift
        if (radial_blast_timer > 0) {
            int new_radius = (8 - radial_blast_timer) * 12;
            // Outer ring: white
            uint16_t c_outer = COLOR_WHITE;
            // Mid ring (80% radius): cyan
            int mid_r = (new_radius * 4) / 5;
            // Inner ring (55% radius): yellow-orange
            int inn_r = (new_radius * 11) / 20;

            // Draw three concentric rectangular rings
            auto draw_ring = [&](int r, uint16_t col) {
                if (r < 1) return;
                draw_rect(radial_blast_x - r, radial_blast_y - r, r * 2, 2,   col);
                draw_rect(radial_blast_x - r, radial_blast_y + r, r * 2, 2,   col);
                draw_rect(radial_blast_x - r, radial_blast_y - r, 2,   r * 2, col);
                draw_rect(radial_blast_x + r, radial_blast_y - r, 2,   r * 2, col);
            };
            draw_ring(new_radius, c_outer);
            draw_ring(mid_r,      C_CYAN);
            draw_ring(inn_r,      C_ORANGE);

            radial_blast_drawn_radius = new_radius;
        }

        // HUD: always drawn in un-shaked mode so it never jiggles
        draw_shake_offset_x = 0;
        draw_shake_offset_y = 0;
        draw_hud_bar();
        draw_lives(lives);
        draw_score(score);

        sleep_ms(30);
    }
}
