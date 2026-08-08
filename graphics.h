#pragma once
#include <stdint.h>
#include "constants.h"
#include "game_types.h"

// Screenshake global offsets declared as extern
extern int shake_offset_x;
extern int shake_offset_y;
extern int draw_shake_offset_x;
extern int draw_shake_offset_y;
extern int high_score;

// Low level SPI commands
void write_cmd(uint8_t cmd);
void write_data(uint8_t data);
void set_addr_window(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1);

// General Drawing
void tft_init();
void draw_rect(int x, int y, int w, int h, uint16_t color);
void draw_digit(int x, int y, int num, uint16_t color);
void draw_text(int x, int y, const char *str, uint16_t color);
void draw_play_icon(int x, int y);
void draw_trophy_icon(int x, int y);

// Sprite Drawing
void draw_sprite(int x, int y, int w, int h, const uint16_t sprite[8][8]);
void draw_sprite_bullet(int x, int y);
void draw_sprite_boss(int x, int y, BossType type);
void draw_sprite_powerup(int x, int y, const uint16_t sprite[8][8]);
void erase_sprite(int x, int y, int w, int h);
void draw_lives(int current_lives);
void draw_score(int score);
void draw_big_digit(int x, int y, int num, uint16_t color);
void draw_gameover_base(int score, int high_score);
void draw_gameover_overlay(bool new_best, int frame);
void draw_hud_bar();
void draw_boss_hp_bar_styled(int hp, int max_hp);
void draw_pause_overlay();
void draw_powerup_badges();
void draw_stats_overlay();
