#pragma once

enum GameState {
    STATE_INTRO,
    STATE_PLAYING,
    STATE_GAMEOVER
};

enum BossType {
    BOSS_MOTHERSHIP,
    BOSS_DREADNOUGHT
};

enum PowerUpType {
    POWERUP_SHIELD,
    POWERUP_DOUBLE,
    POWERUP_BOMB
};

enum EnemyType {
    ENEMY_SCOUT,
    ENEMY_BOMBER,  // moves in sine wave
    ENEMY_CHARGER, // moves fast
    ENEMY_DIVER,   // dives toward player when close
    ENEMY_SHOOTER  // shoots small bullets at player
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
    bool active;
    bool dived;
    int shoot_cooldown;
};

struct PowerUpItem {
    float x;
    int y;
    PowerUpType type;
    bool active;
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
