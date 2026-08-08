#pragma once
#include <stdint.h>
#include "hardware/spi.h"

// SPI Port and Pins
#define SPI_PORT spi0
#define PIN_SCK  18
#define PIN_SDA  19
#define PIN_CS   17
#define PIN_DC   20  // Data/Command (wired to AO)
#define PIN_RST  21

// Button Input Pin
#define BUTTON_PIN 2

// Screen Dimensions
#define SCREEN_WIDTH  128
#define SCREEN_HEIGHT 160

// Color Definitions (RGB565 format)
#define COLOR_BLACK   0x0000
#define COLOR_BLUE    0x001F
#define COLOR_RED     0xF800
#define COLOR_GREEN   0x07E0
#define COLOR_WHITE   0xFFFF
#define COLOR_YELLOW  0xFFE0
#define COLOR_MAGENTA 0xF81F
#define C_TRANS   0x0000
#define C_METAL   0x7BEF // Gray
#define C_ORANGE  0xFD20
#define C_CYAN    0x07FF

// Config parameters
const float GRAVITY = 0.15f;
const float JUMP_FORCE = -2.5f;
const int PLAYER_X = 15;
