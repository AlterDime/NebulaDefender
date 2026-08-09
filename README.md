# Nebula Defender

A fast-paced, 1-button arcade space shooter built for the **Raspberry Pi Pico (RP2040)** microcontroller, designed for custom retro console hardware (such as the PicoNES board) with ST7735 SPI displays and active piezo audio.

Nebula Defender combines arcade flight gravity controls with classic shoot-'em-up mechanics, progressive 10-boss cycle progression, stacking weapon upgrades, smart bomb charge blasts, seeking missiles, laser grid walls, and persistent high-score flash memory storage.

---

## Hardware Setup & Pinout

### Required Components
* **Microcontroller:** Raspberry Pi Pico (RP2040)
* **Display:** 1.8" ST7735 128x160 SPI TFT Display (RGB565 format)
* **Control Input:** Tactile push-button (connected to GPIO 16 with internal pull-up)
* **Audio:** Active Piezo Buzzer (connected to GPIO 15)

### Wiring Pinout Configuration

| Component Pin | RP2040 Pin | Function / Description |
| :--- | :--- | :--- |
| **ST7735 SDA (MOSI)** | GPIO 19 | SPI0 TX Data Line |
| **ST7735 SCK (CLK)** | GPIO 18 | SPI0 Clock Line (15 MHz) |
| **ST7735 CS** | GPIO 17 | SPI Chip Select |
| **ST7735 DC** | GPIO 20 | Data / Command Control |
| **ST7735 RESET** | GPIO 21 | Hardware Reset Line |
| **ST7735 VCC** | 3.3V (Out) / 5V | Power Supply |
| **ST7735 GND** | GND | Ground |
| **Action Button** | GPIO 16 | Player Control (Active Low, Pull-Up) |
| **Piezo Speaker** | GPIO 15 | Audio Tone Output |

---

## Controls & Gameplay Mechanics

Designed specifically for an authentic **1-Button Retro Arcade** experience:

* **Single Tap:** Flap upwards against gravity and fire forward laser projectiles.
* **Continuous Holding:** Activates automatic progressive firing while holding altitude, and charges your **Smart Bomb** (once picked up).
* **Release Button:** Detonates a fully charged **Smart Bomb** (Radial Blast), unleashing a screen-clearing shockwave that destroys standard enemies or deals heavy damage (5 HP) to Bosses.

---

## Power-Up Arsenal

Collecting floating power-up pods unlocks specialized tactical weapons:

* **Shield Matrix:** Deploys an energy barrier that absorbs 1 hit and deflects incoming damage.
* **Dual Cannon:** Upgrades ship weapons to fire twin parallel laser streams.
* **Spread Triple:** Launches a wide 3-way angled laser spread.
* **Hyper Overload:** Triggers hyper-rapid automatic laser storm salvoes.
* **Helper Drone:** Deploys an autonomous wingman drone that orbits the player ship and auto-targets enemies.
* **Seeking Missiles (8s):** Fires homing warheads that dynamically track and steer toward the nearest enemy or boss target.
* **Laser Grid (8s):** Deploys a sweeping vertical laser wall that cuts across the playfield and vaporizes all enemies in its path.
* **Smart Bomb:** Adds a stackable screen-clearing bomb to your arsenal.
* **Repair Ship:** Restores 1 unit of ship hull integrity (+1 Extra Life).

---

## Key Features

1. **Single-Button Flight & Combat Engine:** Flappy-style arcade gravity physics combined with tap/hold auto-fire, homing missiles, laser grids, and charge-up tactical smart bomb mechanics.
2. **Stacking Weapon Level System (Level 1 to 4):**
   * **Level 1 (Starting Weapon):** Low, deliberate fire rate (~2.5 shots/sec) single blaster.
   * **Level 2 (Dual Cannon):** Moderate fire rate twin parallel lasers.
   * **Level 3 (Spread Cannon):** Fast 3-way spread salvoes.
   * **Level 4 (Hyper Quad Salvo):** High-speed 4-stream quad plasma barrage (~7.5 shots/sec).
3. **10-Boss Encounter Progression:** Boss encounters trigger progressively with unique attack patterns and scaled HP:
   1. **Mothership (20 HP):** Dual laser volley.
   2. **Dreadnought (30 HP):** 3-bullet spread salvoes & scout drone launches.
   3. **Viper (40 HP):** Fast 4-bullet fan spread with dodge gaps.
   4. **Phantom (50 HP):** 5-bullet stealth pulse salvoes.
   5. **Titan (60 HP):** 5-bullet heavy wide fan.
   6. **Asteroid (70 HP):** 6-bullet barrage with central gap.
   7. **Dragon (80 HP):** 6-bullet high-velocity fiery fan.
   8. **Chrono (90 HP):** Teleportation phase shifts + 6-bullet barrage.
   9. **Nebula (105 HP):** 7-bullet oscillating sine wave pattern.
   10. **Omega (125 HP):** 7-bullet final boss barrage with wide dodge corridors.
4. **Balanced Scoring & Multipliers:**
   * Enemy kill streak combo timer (1.5s window) with capped **2x maximum multiplier** to keep scoring balanced.
   * Boss defeat bonus (+5 points).
5. **Synchronized Wave Formations:** Enemies spawn in structured wave formations across 3 vertical flight corridors.
6. **Robust Display Protection:** Hard boundary clipping in `graphics.cpp` prevents screenshake camera offsets or bullet erases from bleeding over or corrupting the Top HUD Bar (`Y 0..17`) or Bottom Boss HP Bar (`Y 148..160`).
7. **Flash Memory Persistence:** High scores, boss defeats, games played, and peak combo records persist across power cycles using non-volatile flash sector storage.

---

## Codebase Architecture

Modular C++ codebase structured for resource-constrained microcontrollers:

* [`constants.h`](file:///Users/akshu/Downloads/PicoNES/constants.h) – Standardizes hardware pinouts (SPI0, GPIOs), screen boundaries, gravity, and RGB565 color definitions.
* [`sprites.h`](file:///Users/akshu/Downloads/PicoNES/sprites.h) – Pixel art bitmaps (8x8, 16x16, 42x24) for player ship, enemies, bosses, power-ups, fonts, and icons.
* [`game_types.h`](file:///Users/akshu/Downloads/PicoNES/game_types.h) – Data structures and enums (`Player`, `Bullet`, `Enemy`, `BossBullet`, `EnemyType`, `BossType`, `PowerUpType`).
* [`graphics.h`](file:///Users/akshu/Downloads/PicoNES/graphics.h) / [`graphics.cpp`](file:///Users/akshu/Downloads/PicoNES/graphics.cpp) – Optimized SPI ST7735 TFT display driver, playfield clipping, text rendering, and HUD overlays.
* [`audio.h`](file:///Users/akshu/Downloads/PicoNES/audio.h) / [`audio.cpp`](file:///Users/akshu/Downloads/PicoNES/audio.cpp) – Non-blocking PWM chiptune sound effects engine.
* [`effects.h`](file:///Users/akshu/Downloads/PicoNES/effects.h) / [`effects.cpp`](file:///Users/akshu/Downloads/PicoNES/effects.cpp) – Dynamic multi-layer parallax starfields, particle physics explosions, thrust plumes, and camera screenshake.
* [`game.h`](file:///Users/akshu/Downloads/PicoNES/game.h) / [`game.cpp`](file:///Users/akshu/Downloads/PicoNES/game.cpp) – Game state variables, progressive enemy/power-up unlock logic, flash saving, and reset routines.
* [`main.cpp`](file:///Users/akshu/Downloads/PicoNES/main.cpp) – System initialization, main game loop router, collision detection, and dual-stage render pipeline.

---

## Build & Flash Instructions

### Prerequisites
* **Raspberry Pi Pico SDK** (v2.0+) installed and configured with `PICO_SDK_PATH` environment variable.
* **CMake** (v3.13+) and ARM GCC cross-compiler toolchain (`arm-none-eabi-gcc` / `arm-none-eabi-g++`).

### Build Steps

1. Clone the repository and navigate to the project directory:
   ```bash
   cd PicoNES
   ```

2. Generate build files with CMake:
   ```bash
   cmake -B build -S .
   ```

3. Compile the firmware binary:
   ```bash
   cmake --build build
   ```

Upon successful compilation, the output binaries will be generated in the `build/` directory:
* `build/nebuladefender.uf2` (UF2 bootloader binary)
* `build/nebuladefender.elf` (ELF executable for debugging)

### Flashing to Hardware

1. Connect the Raspberry Pi Pico to your host machine via USB while pressing and holding the **BOOTSEL** button on the Pico board.
2. Release the **BOOTSEL** button once the Pico registers as a mass storage drive named `RPI-RP2`.
3. Drag and drop (or copy) `build/nebuladefender.uf2` onto the `RPI-RP2` volume.
4. The Pico will automatically flash the firmware, reboot, and run Nebula Defender.

---

## Technical Architecture

### 1. Dual-Stage Rendering (Flicker-Free, No Double-Buffer)
Due to RP2040 SRAM constraints (264 KB SRAM vs 40 KB required for a single 16-bit 128x160 frame buffer), Nebula Defender avoids heavy full-frame RAM buffers. Instead, it uses a **coordinate tracking erase-and-draw cycle**:
* **Tracking:** Moving entities store their previous frame's rendered screen position and camera shake in `old_*` arrays.
* **Erase Pass:** The engine erases the exact footprint of previous sprites using `old_*` positions before computing new positions.
* **Draw Pass:** Redraws active entities at their new coordinates with current camera shake. This guarantees 30 FPS flicker-free rendering without tearing.

### 2. Hard Layer Boundary Protection
To prevent screenshake camera offsets or bullet erases from corrupting the fixed HUD overlay, `draw_rect()` in `graphics.cpp` enforces hard playfield boundary clipping (`Y = 18` to `148`). The Top HUD Bar and Bottom Boss HP Bar are rendered in static (un-shaked) mode at the end of every frame.

### 3. Flash Memory Persistence
High scores and stats are persisted in the RP2040's onboard Flash memory:
* **Target Sector:** `#define FLASH_TARGET_OFFSET (2048 * 1024 - 4096)` (the final 4KB sector of 2MB Flash, safely isolated from firmware binaries).
* **Write Protocol:** Disables interrupts (`save_and_disable_interrupts()`), erases the sector (`flash_range_erase()`), programs 256-byte flash pages (`flash_range_program()`), and restores interrupts (`restore_interrupts()`).

---

## License

This project is licensed under the MIT License. See [LICENSE](LICENSE) for details.
