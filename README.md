# Nebula Defender 🚀
A fast-paced, retro-style space shooter built for the **Raspberry Pi Pico** microcontroller, designed to run on custom retro console hardware (like the PicoNES board).

Nebula Defender features arcade flappy-bird style flight physics, responsive graphics rendering, non-blocking chiptune audio, progressive difficulty scaling, double-boss cycles, and flash memory persistence.

---

## 🎮 Gameplay & Controls
* **Tap Button:** Flap upwards against flight gravity and fire forward laser projectiles.
* **Hold Button:** Charges your **Smart Bomb** (Radial Blast) once a bomb power-up is picked up.
* **Release Button:** Detonates the Smart Bomb when fully charged, clearing the screen of standard enemies or dealing massive damage ($5\text{ HP}$) to Bosses.

---

## ✨ Features
1. **Flappy Flight Mechanics:** Gravity pulls your ship down continuously. Flap upwards to navigate lanes and dodge obstacles.
2. **First-Time Tutorials:** When you collect a power-up for the first time (**Shield**, **Double Shot**, or **Smart Bomb**), the game pauses and draws a clean dialog box explaining the new mechanic.
3. **Double Boss Cycle:** A boss wave spawns every $50$ points, alternating between:
   * **The Mothership ($15\text{ HP}$):** Rapid vertical drift; shoots sniper bullets.
   * **The Dreadnought Carrier ($25\text{ HP}$):** Slow movement; shoots triple spread bullets and launches combat drones from its deck.
4. **Locked-Aim Divers:** Divers detect your position, lock onto your Y-coordinate at the dash instant, and charge straight down that trajectory, allowing you to timing-dodge their dashes.
5. **Shooter Aliens:** Standard red/yellow Shooters glide in from the right and fire small yellow projectiles that you must weave around.
6. **Progressive Enemy Unlocking:** Difficulty scales intelligently by unlocking harder enemies (**Scouts** $\rightarrow$ **Bombers** $\rightarrow$ **Chargers** $\rightarrow$ **Divers** $\rightarrow$ **Shooters**) as your score increases, rather than simply scaling game speed.
7. **High-Score Persistence:** High scores are saved directly to the Pico's onboard Flash memory and persist across power-offs.
8. **Juicy Feedback:** Includes dynamic chiptune sfx, 2D particle explosions, engine thrust plumes, and screen shake.

---

## 📁 Repository Structure
The codebase has been refactored from a single monolithic file into a modular structure:
* `constants.h` – Standardizes hardware configurations, screen coordinates, pin-outs, and RGB565 color definitions.
* `sprites.h` – Stores 8x8 and 16x16 pixel-art bitmaps (ship, heart, enemies, bosses, power-ups) and character fonts.
* `game_types.h` – Core structures and enum blueprints (entities, bullets, states).
* `graphics.h` / `graphics.cpp` – Low-level SPI TFT screen drivers, text overlays, and drawing functions.
* `audio.h` / `audio.cpp` – Non-blocking PWM chiptune audio player.
* `effects.h` / `effects.cpp` – Coordinates starfields, particle physics, and camera screenshake.
* `game.h` / `game.cpp` – Holds global variables, progressive selectors, flash saving, and reset states.
* `main.cpp` – Core system configurations, game state routing, and the main game loops.

---

## 🛠️ Build & Flash Instructions

### Prerequisites
Make sure you have the **Raspberry Pi Pico SDK** installed and configured in your environment.

### Compile
1. Create a `build` directory inside the project root folder:
   ```bash
   mkdir build && cd build
   ```
2. Run CMake and build using Ninja/Make:
   ```bash
   cmake ..
   ninja
   ```
3. This generates `blink.uf2` in the build folder.

### Flash
1. Press and hold the **BOOTSEL** button on your Raspberry Pi Pico.
2. Connect the Pico to your computer via USB.
3. Release the button; the Pico will mount as a mass storage device named `RPI-RP2`.
4. Drag and drop `blink.uf2` onto the `RPI-RP2` drive. The Pico will reboot and run the game automatically!

---

## 🧠 Developer Knowledge Transfer (KT) Guide

Welcome to the project! This guide explains the core systems, optimization designs, and algorithms implemented in Nebula Defender to help you get up to speed.

### 1. Code Architecture & Execution Flow
The entry point is `main()` in [main.cpp](file:///Users/akshu/Downloads/PicoNES/blink/main.cpp). The flow operates as follows:
1. **Peripherals Init:** Sets up SDK stdio, SPI at 15MHz (TFT screen communication), GPIO button pins with pull-ups, and the hardware PWM slice for the buzzer.
2. **Flash Load:** Calls `load_high_score()` which reads from the Pico's last 4KB flash sector to populate the `high_score` global variable.
3. **Reset State:** Invokes `reset_game()`, defining player coordinates, resetting enemy speeds, and calling `init_starfield()`.
4. **State Machine Loop:** Runs a 30ms-throttled `while(true)` loop representing the router:
   * **`STATE_INTRO`:** Draws the animated title and waits for GP2 button releases.
   * **`STATE_PLAYING`:** Resolves gravity, ticks entities (bullets, enemies, particles), computes boundaries and overlaps, erases previous coordinates, and draws new frame coordinates.
   * **`STATE_GAMEOVER`:** Updates scores and flash persistence.

### 2. Dual-Stage Rendering (Flicker-Free, No Double-Buffer)
Microcontrollers usually lack enough SRAM for a full double-buffered $128 \times 160 \times 16$-bit ($40\text{ KB}$) frame buffer. To prevent display flickering and visual artifacts, Nebula Defender uses a **coordinate tracking erase-and-draw cycle**:
* **Tracking:** Every moving object caches its drawn coordinate in `old_*` array pointers (e.g. `old_enemy_x/y`).
* **Erase Pass:** At the start of the frame render, the engine writes black rectangles over the `old_*` coordinates, cleanly purging trails.
* **Camera Offset Shift:** Because camera screenshake continuously offsets coordinates, the Erase Pass is forced to use the previous frame's screenshake offset (`o_shake_x/y`) to erase trails. The Draw Pass then calculates the new screenshake offset (`shake_offset_x/y`) to draw the sprites.
* **Draw Pass:** Sprites are redrawn at their current `x/y` coordinates. Because erase and draw occurrences happen in quick succession, no sprite flickering occurs.

### 3. Non-Blocking PWM Audio
The audio synthesizer runs in a non-blocking mode within the main game loop, ensuring sound effects don't stutter frame updates:
* `play_tone(frequency, duration_frames)` initializes the PWM divider for GP15, sets duty cycle to 50% for a clean square wave, and sets `audio_timer = duration_frames`.
* Each tick, `update_audio()` counts down `audio_timer`. Once it reaches zero, it calls `stop_tone()`, silencing GP15.

### 4. Non-Overlapping Flash Memory Persistence
High scores are stored directly in flash memory:
* **Storage Offset:** `#define FLASH_TARGET_OFFSET (2048 * 1024 - 4096)`. This points to the final 4KB sector of a 2MB flash space, well away from the compiled `.uf2` binary.
* **Write Lock:** Flash programming requires blocking interrupts. We disable interrupts using `save_and_disable_interrupts()`, erase the sector via `flash_range_erase()`, write the payload with `flash_range_program()`, and re-enable interrupts via `restore_interrupts()`.

### 5. Enemy Pathing Algorithms
* **Bombers (`ENEMY_BOMBER`):** Y-coordinate varies continuously using a sine wave function matching its current X-coordinate:
  $$\text{y} = \text{base\_y} + \sin(\text{x} \times 0.08) \times 15.0$$
* **Divers (`ENEMY_DIVER`):** When the Diver's X-coordinate enters the Y-lock range (within 65 pixels of the player), it captures the player's Y-coordinate at that exact instant into `base_y` and sets `dived = true`. It then smoothly glides to that static target point using simple interpolation:
  $$\text{y} += (\text{target\_y} - \text{y}) \times 0.1$$
  This allows players to easily dodge the dash by changing Y-altitude right after the lock triggers.

---

## 📄 License
This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.
