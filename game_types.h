#pragma once

enum GameState {
    STATE_INTRO,
    STATE_PLAYING,
    STATE_GAMEOVER
};

enum GameMode {
    MODE_NORMAL,
    MODE_HYPER,
    MODE_BOSSRUSH
};

enum BossType {
    BOSS_MOTHERSHIP,
    BOSS_DREADNOUGHT,
    BOSS_VIPER,
    BOSS_PHANTOM,
    BOSS_TITAN,
    BOSS_ASTEROID,
    BOSS_DRAGON,
    BOSS_CHRONO,
    BOSS_NEBULA,
    BOSS_OMEGA
};

enum PowerUpType {
    POWERUP_SHIELD,
    POWERUP_DOUBLE,
    POWERUP_BOMB,
    POWERUP_LIFE,
    POWERUP_SPREAD,
    POWERUP_DRONE,
    POWERUP_BLACKHOLE,
    POWERUP_OVERLOAD
};

enum EnemyType {
    ENEMY_SCOUT,
    ENEMY_BOMBER,    // moves in sine wave
    ENEMY_CHARGER,   // moves fast
    ENEMY_DIVER,     // dives toward player when close
    ENEMY_SHOOTER,   // shoots small bullets at player
    ENEMY_SHIELDED,  // 2 HP armored ship
    ENEMY_SWARMER,   // fast formation swarmer
    ENEMY_TURRET,    // angled laser turret
    ENEMY_PHANTOM,   // cloaked stealth fighter (pulses invisible)
    ENEMY_MINELAYER, // drops lingering space mines
    ENEMY_BEAMER,    // fires horizontal laser beam
    ENEMY_COMMANDER  // 3 HP dreadnought commander
};

struct Player {
    float y;
    float velocity;
    int width = 8;
    int height = 8;
};

struct Bullet {
    int x;
    int y;
    bool active;
};

struct Enemy {
    float x;
    float y;
    float base_y;
    float speed;
    EnemyType type;
    int hp = 1;
    bool active;
    bool dived;
    int shoot_cooldown;
};

struct PowerUpItem {
    float x;
    int y;
    PowerUpType type;
    bool active;
    int duration_frames = 0; // 0 = standard score scaling duration; >0 = boss long-tier duration
};

struct BossBullet {
    float x;
    float y;
    float vx;
    float vy;
    bool active;
};

struct EnemyBullet {
    float x;
    float y;
    bool active;
};

struct SpaceMine {
    float x;
    float y;
    bool active;
};

struct Gem {
    float x;
    float y;
    bool active;
    int value; // point value (10 or 25)
};

struct HelperDrone {
    float x;
    float y;
    float angle;
    bool active;
    int timer;
    int shoot_cooldown;
};

struct BlackHole {
    float x;
    float y;
    bool active;
    int timer;
    int pulse;
};
