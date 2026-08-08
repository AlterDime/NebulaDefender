#include <stdio.h>
#include <stdlib.h>
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
    int gameover_frame_counter = 0;

    while (true) {
        // Read input (Active LOW)
        bool button_pressed = !gpio_get(BUTTON_PIN);

        // --- STATE: INTRO ---
        if (current_state == STATE_INTRO) {
            if (state_changed) {
                intro_frame_counter = 0;

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
                draw_text(42, 90, "BEST", 0x8400);
                // Big digits centred (2× scale = 10px wide + 3px gap, 3 digits = 36px total)
                draw_big_digit(46, 100, (high_score / 100) % 10, COLOR_YELLOW);
                draw_big_digit(59, 100, (high_score / 10)  % 10, COLOR_YELLOW);
                draw_big_digit(72, 100, high_score % 10,          COLOR_YELLOW);

                // Thin separator above best score
                draw_rect(18, 87, SCREEN_WIDTH - 36, 1, 0x2945);
                // Thin separator below best score
                draw_rect(18, 120, SCREEN_WIDTH - 36, 1, 0x2945);

                old_intro_ship_y = 57;
                state_changed = false;
            }

            intro_frame_counter++;

            // === Pulsing border (top + bottom edges only — subtle) ===
            uint16_t border_col = (intro_frame_counter / 25 % 2 == 0) ? C_CYAN : 0x0294;
            draw_rect(0, 0,              SCREEN_WIDTH, 2, border_col);
            draw_rect(0, SCREEN_HEIGHT - 2, SCREEN_WIDTH, 2, border_col);

            // === Animated ship hovering in the middle zone (Y ~52-62) ===
            // Erase only the exact region: flame (x=6) + ship (x=16+8) + bolt tip (x=34) + 1px margin
            draw_rect(6, old_intro_ship_y - 1, 32, 11, COLOR_BLACK);

            int ship_y = 57 + (int)(sinf(intro_frame_counter * 0.12f) * 5.0f);

            // Multi-pixel flame plume (flickers each frame)
            uint16_t flame1 = (intro_frame_counter % 2 == 0) ? C_ORANGE : COLOR_YELLOW;
            uint16_t flame2 = (intro_frame_counter % 2 == 0) ? COLOR_RED  : C_ORANGE;
            draw_rect(10, ship_y + 2, 5, 2, flame1);
            draw_rect(8,  ship_y + 3, 3, 2, flame2);
            draw_rect(7,  ship_y + 4, 2, 1, COLOR_RED);

            // Ship sprite
            draw_sprite(16, ship_y, 8, 8, sprite_player);

            // Laser bolt in front of ship
            draw_rect(25, ship_y + 3, 5, 2, COLOR_YELLOW);
            draw_rect(30, ship_y + 3, 3, 2, COLOR_WHITE);  // bright tip

            old_intro_ship_y = ship_y;

            // === Dot-chase attract strip (Y=76, safely below ship zone) ===
            {
                int lit = (intro_frame_counter / 8) % 7;
                for (int i = 0; i < 7; i++) {
                    uint16_t dot = (i == lit) ? C_CYAN : 0x0294;
                    draw_rect(20 + i * 14, 76, 8, 3, dot);
                }
            }

            // === Blinking "TAP TO START" prompt (Y 130) ===
            if ((intro_frame_counter / 20) % 2 == 0) {
                // "TAP TO START" = 12 chars × 6px = 72px → start at (128-72)/2 = 28
                draw_text(28, 130, "TAP TO START", C_CYAN);
            } else {
                draw_rect(28, 130, 72, 8, COLOR_BLACK);
            }

            // Start game on button tap
            if (button_pressed && !last_button_state) {
                current_state = STATE_PLAYING;
                state_changed = true;
                reset_game();
                draw_rect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, COLOR_BLACK);
                draw_hud_bar();
                draw_score(score);
                draw_lives(lives);
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
            if (charge_blast_enabled) {
                button_hold_frames++;
                // Rising pitch hum cue when fully charged
                if (button_hold_frames == 25) {
                    play_tone(980, 4); 
                }
            }
        } else if (!button_pressed && last_button_state) {
            // Button released! Check if charge blast is triggered
            if (charge_blast_enabled && button_hold_frames >= 25) {
                // Detonate Radial Smart Bomb!
                radial_blast_timer = 8;
                radial_blast_x = PLAYER_X + player.width / 2;
                radial_blast_y = (int)player.y + player.height / 2;

                // Deactivate power-up (one-time use)
                charge_blast_enabled = false;

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

                        // Re-enable normal enemies
                        for (int i = 0; i < MAX_ENEMIES; i++) {
                            enemies[i].active = true;
                            enemies[i].x = SCREEN_WIDTH + 30 + (i * 55);
                            enemies[i].base_y = 20 + rand() % (SCREEN_HEIGHT - 40);
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

        // Trigger Boss Wave every 50 points
        if (!boss_active && score >= last_boss_score + 50) {
            boss_active = true;
            last_boss_score = (score / 50) * 50; // Anchor last boss score
            boss_encounter_count++;
            
            // Alternate between Mothership and Dreadnought Carrier
            current_boss_type = (boss_encounter_count % 2 == 1) ? BOSS_MOTHERSHIP : BOSS_DREADNOUGHT;
            boss_max_hp = (current_boss_type == BOSS_MOTHERSHIP) ? 15 : 25;
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

        // Apply physics (Hover lock in place when charging smart bomb)
        if (charge_blast_enabled && button_hold_frames >= 5) {
            player.velocity = 0;
        } else {
            player.velocity += GRAVITY;
            player.y += player.velocity;
        }

        // Check boundaries
        if (player.y < 0) {
            player.y = 0;
            player.velocity = 0;
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
                    erase_sprite(old_bullet_x[i], old_bullet_y[i], 8, 4); // Erase bullet
                }
            }
        }

        // Update Boss Mechanics
        int old_boss_x = (int)boss_x;
        int old_boss_y = (int)boss_y;
        if (boss_active) {
            if (current_boss_type == BOSS_MOTHERSHIP) {
                // Mothership movement: drift vertically
                if (boss_x > 100) {
                    boss_x -= 0.5f;
                } else {
                    boss_y += boss_vy;
                    if (boss_y < 15 || boss_y > SCREEN_HEIGHT - 32) {
                        boss_vy = -boss_vy;
                    }
                }

                // Mothership weapon: Single Sniper Bullet
                boss_shoot_timer++;
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
                // Dreadnought Carrier movement: slower, stays at 105
                if (boss_x > 105) {
                    boss_x -= 0.4f;
                } else {
                    boss_y += boss_vy;
                    if (boss_y < 20 || boss_y > SCREEN_HEIGHT - 35) {
                        boss_vy = -boss_vy;
                    }
                }

                boss_shoot_timer++;
                // Dreadnought weapon 1: Triple Spread Shot
                if (boss_shoot_timer % 70 == 0) {
                    int launched = 0;
                    for (int k = 0; k < MAX_BOSS_BULLETS; k++) {
                        if (!boss_bullets[k].active) {
                            boss_bullets[k].active = true;
                            boss_bullets[k].x = boss_x;
                            boss_bullets[k].y = boss_y + 6;
                            boss_bullets[k].vx = (launched == 1) ? -2.4f : -2.1f;
                            boss_bullets[k].vy = (launched == 0) ? -0.5f : ((launched == 1) ? 0.0f : 0.5f);
                            launched++;
                            if (launched >= 3) break;
                        }
                    }
                    if (launched > 0) {
                        play_tone(440, 4);
                    }
                }

                // Dreadnought weapon 2: Launch Drone Fighters!
                if (boss_shoot_timer >= 140) {
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
                        enemies[i].base_y = 20 + rand() % (SCREEN_HEIGHT - 40);
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
                        enemies[i].y = enemies[i].base_y + sinf(enemies[i].x * 0.08f) * 15.0f;
                    }
                    
                    // Homing dive path for Diver type enemies when within proximity
                    if (enemies[i].type == ENEMY_DIVER) {
                        if (!enemies[i].dived && enemies[i].x < PLAYER_X + 65 && enemies[i].x > PLAYER_X) {
                            enemies[i].dived = true;
                            enemies[i].base_y = player.y; // Lock to the player's Y coordinate at initiation
                        }
                        if (enemies[i].dived) {
                            enemies[i].y += (enemies[i].base_y - enemies[i].y) * 0.1f;
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
                        erase_sprite(old_enemy_x[i], old_enemy_y[i], 8, 8); // Erase old position
                    }
                }
            }
        }

        // Update Power Up Item (Erase uses o_shake)
        int old_powerup_x = (int)power_up.x;
        if (power_up.active) {
            power_up.x -= 1.0f;
            if (power_up.x < -10) {
                power_up.active = false;
                erase_sprite(old_powerup_x, power_up.y, 6, 6);
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

                        // Trigger Particle Blast & camera screenshake
                        spawn_explosion((int)enemies[e].x, (int)enemies[e].y);
                        trigger_screenshake(2, 4);

                        // Chance to drop a powerup at enemy's location (25% chance)
                        if (!power_up.active && (rand() % 4 == 0)) {
                            power_up.active = true;
                            power_up.x = enemies[e].x;
                            power_up.y = enemies[e].y;
                            power_up.type = (rand() % 3 == 0) ? POWERUP_SHIELD : ((rand() % 2 == 0) ? POWERUP_DOUBLE : POWERUP_BOMB);
                        }

                        erase_sprite((int)enemies[e].x, (int)enemies[e].y, 8, 8); // Clear enemy sprite
                        enemies[e].x = SCREEN_WIDTH + rand() % 30;
                        enemies[e].base_y = 20 + rand() % (SCREEN_HEIGHT - 40);
                        enemies[e].y = enemies[e].base_y;
                        
                        // Level Progression Speed Scaling
                        enemies[e].speed = 0.8f + (score * 0.005f) + (rand() % 80) / 100.0f;
                        
                        if (enemies[e].type == ENEMY_CHARGER) {
                            enemies[e].speed *= 1.4f;
                        }

                        score += 1;
                        
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
                            enemies[i].base_y = 20 + rand() % (SCREEN_HEIGHT - 40);
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
            if (PLAYER_X + player.width >= power_up.x && PLAYER_X <= power_up.x + 6 &&
                (int)player.y + player.height >= power_up.y && (int)player.y <= power_up.y + 6) {
                
                power_up.active = false;
                erase_sprite((int)power_up.x, power_up.y, 6, 6);
                
                // Play retro high-pitched chime sound
                play_tone(1200, 4);

                if (power_up.type == POWERUP_SHIELD) {
                    shield_active = true;
                    if (!tutorial_shield_done) {
                        tutorial_shield_done = true;
                        show_tutorial_overlay("SHIELD", "ABSORBS ONE HIT");
                    }
                } else if (power_up.type == POWERUP_DOUBLE) {
                    double_shot_active = true;
                    if (!tutorial_double_done) {
                        tutorial_double_done = true;
                        show_tutorial_overlay("DOUBLE SHOT", "DUAL STREAM GUN");
                    }
                } else {
                    charge_blast_enabled = true;
                    if (!tutorial_bomb_done) {
                        tutorial_bomb_done = true;
                        show_tutorial_overlay("SMART BOMB", "HOLD BTN TO CHARGE");
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
        
        // 2. Erase Bullets (cover 8x4 sprite + 1px margin each side)
        for (int i = 0; i < MAX_BULLETS; i++) {
            if (bullets[i].active) {
                erase_sprite(old_bullet_x[i] - 1, old_bullet_y[i] - 1, 10, 6);
            }
        }

        // 3. Erase Enemies or Boss
        if (!boss_active) {
            for (int i = 0; i < MAX_ENEMIES; i++) {
                if (enemies[i].active) {
                    // +2px margin each side catches sinusoidal sub-pixel drift trails
                    erase_sprite(old_enemy_x[i] - 1, old_enemy_y[i] - 1, 10, 10);
                }
            }
        } else {
            // Erase Boss ship (extra 2px margin)
            erase_sprite(old_boss_x - 2, old_boss_y - 2, 20, 20);
            // Erase Boss bullets (8x6 covers 4x4 core + orange trail)
            for (int k = 0; k < MAX_BOSS_BULLETS; k++) {
                erase_sprite(old_boss_bullet_x[k] - 1, old_boss_bullet_y[k] - 1, 8, 6);
                old_boss_bullet_x[k] = 0;
                old_boss_bullet_y[k] = 0;
            }
        }

        // 4. Erase Power-up
        if (power_up.active) {
            erase_sprite(old_powerup_x, power_up.y, 6, 6);
        }

        // 4b. Erase Enemy Bullets (5x5 covers magenta 3x3 + orange trail pixel)
        for (int k = 0; k < MAX_ENEMY_BULLETS; k++) {
            erase_sprite(old_enemy_bullet_x[k] - 1, old_enemy_bullet_y[k] - 1, 6, 5);
            old_enemy_bullet_x[k] = 0;
            old_enemy_bullet_y[k] = 0;
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

        // Starfield and explosion particles bypass global draw offset and are updated with old/new offsets manually
        update_and_draw_starfield(o_shake_x, o_shake_y, shake_offset_x, shake_offset_y);
        update_and_draw_particles(o_shake_x, o_shake_y, shake_offset_x, shake_offset_y);

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

        // Gradient charge bar: green -> yellow -> white, glows when full
        if (charge_blast_enabled && button_hold_frames > 0) {
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
            }
        }

        // 3. Draw Enemies or Boss
        if (!boss_active) {
            for (int i = 0; i < MAX_ENEMIES; i++) {
                if (enemies[i].active) {
                    draw_sprite((int)enemies[i].x, (int)enemies[i].y, 8, 8, 
                        (enemies[i].type == ENEMY_DIVER) ? sprite_enemy_diver : 
                        ((enemies[i].type == ENEMY_SHOOTER) ? sprite_enemy_shooter : sprite_enemy));
                }
            }
        } else {
            // Draw Boss by type
            draw_sprite_boss((int)boss_x, (int)boss_y, current_boss_type);
            
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
                }
            }

            // Styled gradient boss HP bar
            draw_shake_offset_x = 0;
            draw_shake_offset_y = 0;
            draw_boss_hp_bar_styled(boss_hp, boss_max_hp);
            draw_shake_offset_x = shake_offset_x;
            draw_shake_offset_y = shake_offset_y;
        }

        // 4. Draw Power-up Item
        if (power_up.active) {
            draw_sprite_powerup((int)power_up.x, power_up.y, 
                power_up.type == POWERUP_SHIELD ? sprite_powerup_shield : 
                (power_up.type == POWERUP_DOUBLE ? sprite_powerup_double : sprite_powerup_bomb));
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
