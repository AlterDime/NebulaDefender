#include "graphics.h"
#include "sprites.h"
#include <cstring>
#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "hardware/gpio.h"

// Define the global shake offsets declared in graphics.h
int shake_offset_x = 0;
int shake_offset_y = 0;
int draw_shake_offset_x = 0;
int draw_shake_offset_y = 0;

void write_cmd(uint8_t cmd) {
    gpio_put(PIN_DC, 0); // Command mode
    gpio_put(PIN_CS, 0);
    spi_write_blocking(SPI_PORT, &cmd, 1);
    gpio_put(PIN_CS, 1);
}

void write_data(uint8_t data) {
    gpio_put(PIN_DC, 1); // Data mode
    gpio_put(PIN_CS, 0);
    spi_write_blocking(SPI_PORT, &data, 1);
    gpio_put(PIN_CS, 1);
}

void set_addr_window(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1) {
    write_cmd(0x2A); // Column address set
    write_data(0x00);
    write_data(x0);
    write_data(0x00);
    write_data(x1);

    write_cmd(0x2B); // Row address set
    write_data(0x00);
    write_data(y0);
    write_data(0x00);
    write_data(y1);

    write_cmd(0x2C); // Write to RAM command
}

void draw_rect(int x, int y, int w, int h, uint16_t color) {
    // Apply camera screenshake offset
    x += draw_shake_offset_x;
    y += draw_shake_offset_y;

    // Clip negative coordinates
    if (x < 0) {
        w += x;
        x = 0;
    }
    if (y < 0) {
        h += y;
        y = 0;
    }

    if (x >= SCREEN_WIDTH || y >= SCREEN_HEIGHT) return;
    if (x + w > SCREEN_WIDTH) w = SCREEN_WIDTH - x;
    if (y + h > SCREEN_HEIGHT) h = SCREEN_HEIGHT - y;
    if (w <= 0 || h <= 0) return;

    set_addr_window(x, y, x + w - 1, y + h - 1);

    gpio_put(PIN_DC, 1);
    gpio_put(PIN_CS, 0);

    uint8_t high = color >> 8;
    uint8_t low = color & 0xFF;

    // Buffer to speed up transmission
    uint8_t chunk[256];
    for (int i = 0; i < 128; i++) {
        chunk[2 * i] = high;
        chunk[2 * i + 1] = low;
    }

    int total_pixels = w * h;
    while (total_pixels > 0) {
        int to_send = total_pixels > 128 ? 128 : total_pixels;
        spi_write_blocking(SPI_PORT, chunk, to_send * 2);
        total_pixels -= to_send;
    }
    gpio_put(PIN_CS, 1);
}

void tft_init() {
    // Hardware reset
    gpio_put(PIN_RST, 1);
    sleep_ms(10);
    gpio_put(PIN_RST, 0);
    sleep_ms(50);
    gpio_put(PIN_RST, 1);
    sleep_ms(120);

    write_cmd(0x01); // Software Reset
    sleep_ms(150);

    write_cmd(0x11); // Exit Sleep
    sleep_ms(120);

    write_cmd(0x3A); // Set Color format to 16-bit
    write_data(0x05);

    write_cmd(0x36); // Memory Access Control (Orientation)
    write_data(0xC8); // BGR color ordering, standard orientation

    write_cmd(0x29); // Turn Display On
    sleep_ms(100);

    // Clear Screen
    draw_rect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, COLOR_BLACK);
}

void draw_digit(int x, int y, int num, uint16_t color) {
    if (num < 0 || num > 9) return;
    for (int col = 0; col < 5; col++) {
        uint8_t line = font_digits[num][col];
        for (int row = 0; row < 8; row++) {
            if (line & (1 << row)) {
                draw_rect(x + col, y + row, 1, 1, color);
            } else {
                draw_rect(x + col, y + row, 1, 1, COLOR_BLACK);
            }
        }
    }
}

void draw_text(int x, int y, const char *str, uint16_t color) {
    while (*str) {
        char c = *str;
        if (c >= 'A' && c <= 'Z') {
            int index = c - 'A';
            for (int col = 0; col < 5; col++) {
                uint8_t line = font_letters[index][col];
                for (int row = 0; row < 8; row++) {
                    if (line & (1 << row)) {
                        draw_rect(x + col, y + row, 1, 1, color);
                    }
                }
            }
        } else if (c >= '0' && c <= '9') {
            int index = c - '0';
            for (int col = 0; col < 5; col++) {
                uint8_t line = font_digits[index][col];
                for (int row = 0; row < 8; row++) {
                    if (line & (1 << row)) {
                        draw_rect(x + col, y + row, 1, 1, color);
                    }
                }
            }
        } else if (c == ':') {
            draw_rect(x + 2, y + 2, 1, 1, color);
            draw_rect(x + 2, y + 5, 1, 1, color);
        }
        x += 6;
        str++;
    }
}

void draw_sprite(int x, int y, int w, int h, const uint16_t sprite[8][8]) {
    for (int row = 0; row < h; row++) {
        for (int col = 0; col < w; col++) {
            uint16_t color = sprite[row][col];
            if (color != C_TRANS) {
                draw_rect(x + col, y + row, 1, 1, color);
            }
        }
    }
}

void draw_sprite_bullet(int x, int y) {
    for (int row = 0; row < 4; row++) {
        for (int col = 0; col < 8; col++) {
            uint16_t color = sprite_bullet[row][col];
            if (color != C_TRANS) {
                draw_rect(x + col, y + row, 1, 1, color);
            }
        }
    }
}

void draw_sprite_boss(int x, int y, BossType type) {
    for (int row = 0; row < 16; row++) {
        for (int col = 0; col < 16; col++) {
            uint16_t color = (type == BOSS_MOTHERSHIP) ? sprite_boss[row][col] : sprite_boss_dreadnought[row][col];
            if (color != C_TRANS) {
                draw_rect(x + col, y + row, 1, 1, color);
            }
        }
    }
}

void draw_sprite_powerup(int x, int y, const uint16_t sprite[6][6]) {
    for (int row = 0; row < 6; row++) {
        for (int col = 0; col < 6; col++) {
            uint16_t color = sprite[row][col];
            if (color != C_TRANS) {
                draw_rect(x + col, y + row, 1, 1, color);
            }
        }
    }
}

void erase_sprite(int x, int y, int w, int h) {
    draw_rect(x, y, w, h, COLOR_BLACK);
}

void draw_play_icon(int x, int y) {
    for (int col = 0; col < 16; col++) {
        int height = col;
        draw_rect(x + col, y - height / 2, 1, height + 1, COLOR_GREEN);
    }
}

void draw_trophy_icon(int x, int y) {
    draw_rect(x, y, 16, 8, COLOR_YELLOW);       // Cup top
    draw_rect(x + 6, y + 8, 4, 6, COLOR_YELLOW); // Stem
    draw_rect(x + 3, y + 14, 10, 2, COLOR_YELLOW); // Base
}

// Draw the HUD background panel — call once on state entry and when needed
void draw_hud_bar() {
    // Dark navy panel strip (top 18px)
    draw_rect(0, 0, SCREEN_WIDTH, 18, 0x0821);
    // Bright bottom edge line — neon cyan divider
    draw_rect(0, 17, SCREEN_WIDTH, 1, 0x0294);
    // Dim inner shadow line just above divider
    draw_rect(0, 16, SCREEN_WIDTH, 1, 0x0108);
}

void draw_lives(int current_lives) {
    // Clear lives area on the HUD panel
    draw_rect(88, 3, 36, 13, 0x0821);
    for (int i = 0; i < current_lives; i++) {
        int hx = 90 + (i * 10);
        int hy = 6;
        for (int row = 0; row < 5; row++) {
            for (int col = 0; col < 5; col++) {
                uint16_t color = sprite_heart[row][col];
                if (color != C_TRANS) {
                    draw_rect(hx + col, hy + row, 1, 1, color);
                }
            }
        }
    }
}

void draw_score(int score) {
    // "SC" micro-label — dim cyan
    draw_rect(3, 3, 10, 5, 0x0821); // clear
    draw_rect(3, 3, 1, 4, 0x03EF);  // S left
    draw_rect(3, 3, 3, 1, 0x03EF);  // S top
    draw_rect(3, 5, 3, 1, 0x03EF);  // S mid
    draw_rect(5, 5, 1, 4, 0x03EF);  // S right
    draw_rect(3, 8, 3, 1, 0x03EF);  // S bottom
    draw_rect(7, 3, 1, 6, 0x03EF);  // C spine
    draw_rect(7, 3, 3, 1, 0x03EF);  // C top
    draw_rect(7, 8, 3, 1, 0x03EF);  // C bottom

    // Score digits — bright white, shifted down slightly
    int hundreds = (score / 100) % 10;
    int tens     = (score / 10)  % 10;
    int ones     =  score        % 10;
    draw_digit(3,  9, hundreds, COLOR_WHITE);
    draw_digit(9,  9, tens,     COLOR_WHITE);
    draw_digit(15, 9, ones,     COLOR_WHITE);

    // "HI" micro-label — dim yellow, centred
    draw_rect(44, 3, 20, 5, 0x0821); // clear
    draw_rect(44, 3, 1, 6, 0xC600);  // H left
    draw_rect(44, 5, 3, 1, 0xC600);  // H cross
    draw_rect(46, 3, 1, 6, 0xC600);  // H right
    draw_rect(49, 3, 1, 6, 0xC600);  // I

    // Hi-score digits — warm yellow
    int hi_hundreds = (high_score / 100) % 10;
    int hi_tens     = (high_score / 10)  % 10;
    int hi_ones     =  high_score        % 10;
    draw_digit(44, 9, hi_hundreds, COLOR_YELLOW);
    draw_digit(50, 9, hi_tens,     COLOR_YELLOW);
    draw_digit(56, 9, hi_ones,     COLOR_YELLOW);
}

// Styled boss HP bar: bordered, gradient-colored, with BOSS label
void draw_boss_hp_bar_styled(int hp, int max_hp) {
    const int bx = 24, by = 150, bw = 80, bh = 6;

    // Background (dark fill)
    draw_rect(bx - 1, by - 1, bw + 2, bh + 2, 0x2104);
    // Border
    draw_rect(bx - 1, by - 1, bw + 2, 1,      0xF800); // top
    draw_rect(bx - 1, by + bh, bw + 2, 1,      0xF800); // bottom
    draw_rect(bx - 1, by - 1, 1,      bh + 2, 0xF800); // left
    draw_rect(bx + bw, by - 1, 1,      bh + 2, 0xF800); // right

    // Compute fill width
    int fill_w = (hp * bw) / max_hp;
    if (fill_w > bw) fill_w = bw;
    if (fill_w < 0)  fill_w = 0;

    // Gradient: green (full) → yellow (half) → red (low)
    uint16_t bar_color;
    int pct = (hp * 100) / max_hp;
    if      (pct > 60) bar_color = 0x07E0; // green
    else if (pct > 30) bar_color = 0xFFE0; // yellow
    else               bar_color = 0xF800; // red

    // Fill bar
    draw_rect(bx, by, fill_w, bh, bar_color);
    // Clear remainder
    draw_rect(bx + fill_w, by, bw - fill_w, bh, 0x1082);

    // "BOSS" text label left of bar
    draw_rect(0, by - 1, 22, bh + 2, COLOR_BLACK); // clear label zone
    draw_text(1, by, "BOSS", 0xF800);
}

// Draw a digit at 2x scale — each pixel becomes a 2×2 block (10×16 px per digit)
void draw_big_digit(int x, int y, int num, uint16_t color) {
    if (num < 0 || num > 9) return;
    for (int col = 0; col < 5; col++) {
        uint8_t line = font_digits[num][col];
        for (int row = 0; row < 8; row++) {
            uint16_t c = (line & (1 << row)) ? color : COLOR_BLACK;
            draw_rect(x + col * 2, y + row * 2, 2, 2, c);
        }
    }
}

// Static Game Over layout — draw once when entering the state.
// Draws background, header band, score box, and best-score row.
void draw_gameover_base(int score, int high_score) {
    // === Full background: clean black ===
    draw_rect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, COLOR_BLACK);

    // === Header band (Y 0–30) — deep crimson with bright edge lines ===
    draw_rect(0, 0,  SCREEN_WIDTH, 30, 0x5000); // dark crimson fill
    draw_rect(0, 0,  SCREEN_WIDTH,  1, COLOR_RED);  // bright top edge
    draw_rect(0, 29, SCREEN_WIDTH,  1, 0xFC00);     // orange-red bottom edge

    // "GAME" on first line, "OVER" offset right to create stagger effect
    draw_text(28, 5,  "GAME", COLOR_WHITE);
    draw_text(28, 6,  "GAME", COLOR_WHITE);  // +1px Y for boldness
    draw_text(64, 5,  "OVER", COLOR_RED);
    draw_text(64, 6,  "OVER", COLOR_RED);

    // Subtitle in dim cyan below title
    draw_text(16, 19, "NEBULA DEFENDER", 0x034B);

    // === Cyan neon divider under header ===
    draw_rect(0,  32, SCREEN_WIDTH, 1, C_CYAN);
    draw_rect(0,  33, SCREEN_WIDTH, 1, 0x0155); // dim shadow

    // === "YOUR SCORE" label, grey and centred ===
    // "YOUR SCORE" = 10 chars × 6px = 60px → start at (128-60)/2 = 34
    draw_text(34, 38, "YOUR SCORE", 0x7BEF);

    // === BIG score digits (2× scale, 10px wide + 3px gap each) ===
    // 3 digits → total width = 3×10 + 2×3 = 36 → start at (128-36)/2 = 46
    draw_big_digit(46,      47, (score / 100) % 10, COLOR_WHITE);
    draw_big_digit(59,      47, (score / 10)  % 10, COLOR_WHITE);
    draw_big_digit(72,      47, score % 10,          COLOR_WHITE);

    // Thin dim separator under score
    draw_rect(12, 68, SCREEN_WIDTH - 24, 1, 0x2945);

    // === BEST score row: small trophy + label + digits ===
    // Trophy icon at left (16×16)
    draw_rect(10, 73, 16, 16, COLOR_BLACK); // clear trophy area
    draw_trophy_icon(10, 73);

    // "BEST" label
    draw_text(34, 77, "BEST", COLOR_YELLOW);

    // Best score digits (normal scale, right side)
    draw_digit(85, 77, (high_score / 100) % 10, COLOR_YELLOW);
    draw_digit(91, 77, (high_score / 10)  % 10, COLOR_YELLOW);
    draw_digit(97, 77, high_score % 10,          COLOR_YELLOW);

    // Thin dim separator below best row
    draw_rect(12, 92, SCREEN_WIDTH - 24, 1, 0x2945);

    // === Bottom neon line above prompt area ===
    draw_rect(0, 113, SCREEN_WIDTH, 1, C_CYAN);
    draw_rect(0, 114, SCREEN_WIDTH, 1, 0x0155);
}

// Animated Game Over overlay — call every frame with the current frame counter.
// Redraws only the animated elements: border pulse, NEW BEST badge,
// blinking prompt, and indicator dots.
void draw_gameover_overlay(bool new_best, int frame) {
    // === Pulsing border: top + bottom lines alternate red ↔ orange-red ===
    uint16_t border_col = (frame / 20 % 2 == 0) ? COLOR_RED : 0xFC00;
    draw_rect(0, 0,           SCREEN_WIDTH, 1, border_col);
    draw_rect(0, SCREEN_HEIGHT - 1, SCREEN_WIDTH, 1, border_col);

    // === NEW BEST badge (Y 95–108) ===
    if (new_best) {
        bool show = (frame < 120) ? ((frame / 8) % 2 == 0) : true;
        if (show) {
            draw_rect(22, 95, 84, 14, 0x6300);    // dark-gold fill
            draw_rect(22, 95, 84,  1, COLOR_YELLOW); // top edge
            draw_rect(22,108, 84,  1, COLOR_YELLOW); // bottom edge
            draw_rect(22, 95,  1, 14, COLOR_YELLOW); // left edge
            draw_rect(105, 95, 1, 14, COLOR_YELLOW); // right edge
            // "NEW BEST!" — centred in badge
            // 8 chars + ! = 9 × 6 = 54px → start at (128-54)/2 = 37
            draw_text(37, 99, "NEW BEST!", COLOR_YELLOW);
        } else {
            draw_rect(22, 95, 84, 14, COLOR_BLACK);
        }
    }

    // === Blinking "PRESS TO RETRY" prompt (Y 118–126) ===
    if ((frame / 18) % 2 == 0) {
        // "PRESS TO RETRY" = 14 chars × 6 = 84px → start at (128-84)/2 = 22
        draw_text(22, 119, "PRESS TO RETRY", C_CYAN);
    } else {
        draw_rect(22, 119, 84, 8, COLOR_BLACK);
    }

    // === Animated dot indicator strip (Y 133–136) ===
    // 5 dots, one lit at a time cycling with the frame
    int lit = (frame / 12) % 5;
    for (int i = 0; i < 5; i++) {
        uint16_t dot_col = (i == lit) ? C_CYAN : 0x0294;
        draw_rect(44 + i * 9, 133, 5, 4, dot_col);
    }
}
