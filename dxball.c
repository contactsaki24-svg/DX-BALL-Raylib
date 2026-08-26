/* ============================================================================
   DX-Ball (Raylib Edition)
   CSE 102 : Structured Programming Sessional -- Term Project

   A classic brick-breaker rebuilt in pure C using raylib. Single translation
   unit, struct/array based, no dynamic allocation. See README.md for build
   instructions and assets/audio/README.md for the dual-music-mode assets.

   Controls
     LEFT / RIGHT ......... move paddle
     SPACE ................ launch the ball
     M ..................... toggle Music Mode (Professional <-> Bangla Fun)
     P / ENTER ............ pause / confirm menus (see on-screen prompts)
   ==========================================================================*/

#include "raylib.h"
#include "raymath.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

/* -------------------------------------------------------------------------
   1. CONSTANTS & SETUP
   -------------------------------------------------------------------------*/
#define SCREEN_W 800
#define SCREEN_H 650
#define LENGTH(arr) (sizeof(arr) / sizeof((arr)[0]))

#define BRICK_ROWS 6
#define BRICK_COLS 10
#define BRICK_W 70
#define BRICK_H 24
#define BRICK_GAP 6
#define BRICK_TOP 84

#define PADDLE_W 110.0f
#define PADDLE_H 16.0f
#define PADDLE_BOTTOM_GAP 36.0f
#define PADDLE_SPEED 480.0f
#define PADDLE_EXPAND_MULT 1.6f
#define PADDLE_EXPAND_TIME 8.0f

#define BALL_RADIUS 8.0f
#define BASE_BALL_SPEED 320.0f
#define BALL_SPEED_PER_LEVEL 32.0f
#define MAX_BOUNCE_ANGLE (65.0f * DEG2RAD)
#define MAX_BALLS 3

#define MAX_POWERUPS 12
#define POWERUP_SIZE 20.0f
#define POWERUP_SPEED 170.0f
#define POWERUP_DROP_PCT 18 /* percent chance a breakable brick drops one */

#define MAX_PARTICLES 120
#define START_LIVES 3
#define MAX_LEVEL 3

/* -------------------------------------------------------------------------
   2. TYPES
   -------------------------------------------------------------------------*/
typedef enum {
  STATE_MENU,
  STATE_PLAYING,
  STATE_PAUSED,
  STATE_LEVEL_CLEAR,
  STATE_GAME_OVER,
  STATE_VICTORY
} GameState;

typedef enum { MODE_CLASSICAL, MODE_BANGLA } MusicMode;
typedef enum { POWERUP_EXPAND, POWERUP_MULTIBALL } PowerKind;

typedef struct {
  Rectangle rect;
  float expandTimer; /* > 0 while the Expand Paddle power-up is active */
} Paddle;

typedef struct {
  Vector2 pos;
  Vector2 vel;
  bool active;
  bool attached; /* resting on the paddle, waiting for SPACE to launch */
} Ball;

typedef struct {
  Rectangle rect;
  int hp;
  int maxHp;
  bool alive;
  bool unbreakable;
} Brick;

typedef struct {
  Vector2 pos;
  PowerKind kind;
  bool active;
} PowerUp;

typedef struct {
  Vector2 pos, vel;
  float life, maxLife;
  Color color;
  bool active;
} Particle;

/* -------------------------------------------------------------------------
   3. GLOBAL PLAY-FIELD DATA
   -------------------------------------------------------------------------*/
static const Color kRowColors[BRICK_ROWS] = {
    {214, 40, 40, 255},  {247, 127, 0, 255},  {252, 191, 73, 255},
    {124, 196, 116, 255}, {83, 158, 224, 255}, {158, 98, 204, 255},
};

Brick bricks[BRICK_ROWS][BRICK_COLS];
Ball balls[MAX_BALLS];
PowerUp powerups[MAX_POWERUPS];
Particle particles[MAX_PARTICLES];

/* -------------------------------------------------------------------------
   4. LEVEL / ENTITY HELPERS
   -------------------------------------------------------------------------*/
static float BrickGridLeft(void) {
  float gridW = BRICK_COLS * BRICK_W + (BRICK_COLS - 1) * BRICK_GAP;
  return (SCREEN_W - gridW) / 2.0f;
}

/* Builds the brick layout for a given level. Difficulty ramps up via
   thicker (multi-hit) bricks near the top and a handful of unbreakable
   barrier bricks scattered in, per the "Difficulty increases with game
   progression" requirement. */
void BuildLevel(int level) {
  float left = BrickGridLeft();
  for (int r = 0; r < BRICK_ROWS; r++) {
    for (int c = 0; c < BRICK_COLS; c++) {
      Brick *b = &bricks[r][c];
      b->rect = (Rectangle){left + c * (BRICK_W + BRICK_GAP),
                             BRICK_TOP + r * (BRICK_H + BRICK_GAP), BRICK_W,
                             BRICK_H};
      b->unbreakable = false;
      b->alive = true;

      int hp = 1;
      if (level == 2 && r < 2) hp = 2;
      if (level >= 3) {
        if (r < 2) hp = 3;
        else if (r < 4) hp = 2;
      }
      b->hp = hp;
      b->maxHp = hp;
    }
  }

  int barriers = (level == 2) ? 5 : (level >= 3 ? 9 : 0);
  for (int i = 0; i < barriers; i++) {
    int r = GetRandomValue(0, BRICK_ROWS - 1);
    int c = GetRandomValue(0, BRICK_COLS - 1);
    bricks[r][c].unbreakable = true;
    bricks[r][c].alive = true;
  }
}

int CountBreakableAlive(void) {
  int n = 0;
  for (int r = 0; r < BRICK_ROWS; r++)
    for (int c = 0; c < BRICK_COLS; c++)
      if (bricks[r][c].alive && !bricks[r][c].unbreakable) n++;
  return n;
}

void ResetPaddle(Paddle *p) {
  p->rect = (Rectangle){SCREEN_W / 2.0f - PADDLE_W / 2.0f,
                         SCREEN_H - PADDLE_BOTTOM_GAP, PADDLE_W, PADDLE_H};
  p->expandTimer = 0.0f;
}

/* Puts a single ball back on the paddle, attached, ready to launch. */
void RackBall(Paddle *p) {
  for (int i = 0; i < MAX_BALLS; i++) balls[i].active = false;
  balls[0].active = true;
  balls[0].attached = true;
  balls[0].pos = (Vector2){p->rect.x + p->rect.width / 2, p->rect.y - BALL_RADIUS - 1};
  balls[0].vel = Vector2Zero();
  for (int i = 0; i < MAX_POWERUPS; i++) powerups[i].active = false;
}

void LaunchBall(Ball *b, float speed) {
  b->attached = false;
  float angle = ((float)GetRandomValue(-30, 30)) * DEG2RAD;
  b->vel = (Vector2){speed * sinf(angle), -speed * cosf(angle)};
}

/* Splits the first active ball into up to MAX_BALLS total, fanned out at
   even angles around its current direction. */
void SpawnMultiBall(float speed) {
  int src = -1;
  for (int i = 0; i < MAX_BALLS; i++)
    if (balls[i].active && !balls[i].attached) { src = i; break; }
  if (src < 0) return; /* nothing in flight yet (e.g. ball still on paddle) */

  Vector2 base = Vector2Normalize(balls[src].vel);
  const float spread[MAX_BALLS] = {-20.0f * DEG2RAD, 0.0f, 20.0f * DEG2RAD};
  int k = 0;
  for (int i = 0; i < MAX_BALLS; i++) {
    if (balls[i].active) continue; /* fill only empty slots */
    float a = atan2f(base.x, -base.y) + spread[k % (int)LENGTH(spread)];
    balls[i].active = true;
    balls[i].attached = false;
    balls[i].pos = balls[src].pos;
    balls[i].vel = (Vector2){speed * sinf(a), -speed * cosf(a)};
    k++;
  }
}

void SpawnParticles(Vector2 at, Color color, int count) {
  int spawned = 0;
  for (int i = 0; i < MAX_PARTICLES && spawned < count; i++) {
    if (particles[i].active) continue;
    float a = (float)GetRandomValue(0, 359) * DEG2RAD;
    float speed = (float)GetRandomValue(60, 220);
    particles[i].pos = at;
    particles[i].vel = (Vector2){cosf(a) * speed, sinf(a) * speed};
    particles[i].maxLife = particles[i].life = 0.35f + GetRandomValue(0, 20) / 100.0f;
    particles[i].color = color;
    particles[i].active = true;
    spawned++;
  }
}

void SpawnPowerUp(Vector2 at) {
  if (GetRandomValue(1, 100) > POWERUP_DROP_PCT) return;
  for (int i = 0; i < MAX_POWERUPS; i++) {
    if (powerups[i].active) continue;
    powerups[i].active = true;
    powerups[i].pos = at;
    powerups[i].kind = (GetRandomValue(0, 1) == 0) ? POWERUP_EXPAND : POWERUP_MULTIBALL;
    return;
  }
}

void ApplyPowerUp(PowerKind kind, Paddle *paddle, float ballSpeed) {
  if (kind == POWERUP_EXPAND) {
    paddle->expandTimer = PADDLE_EXPAND_TIME;
  } else {
    SpawnMultiBall(ballSpeed);
  }
}

/* -------------------------------------------------------------------------
   5. PHYSICS UPDATE
   -------------------------------------------------------------------------*/
void UpdatePaddle(Paddle *p, float dt) {
  if (IsKeyDown(KEY_LEFT)) p->rect.x -= PADDLE_SPEED * dt;
  if (IsKeyDown(KEY_RIGHT)) p->rect.x += PADDLE_SPEED * dt;
  if (p->rect.x < 0) p->rect.x = 0;
  if (p->rect.x + p->rect.width > SCREEN_W) p->rect.x = SCREEN_W - p->rect.width;

  float targetW = PADDLE_W * ((p->expandTimer > 0) ? PADDLE_EXPAND_MULT : 1.0f);
  if (fabsf(targetW - p->rect.width) > 0.5f) {
    float cx = p->rect.x + p->rect.width / 2;
    p->rect.width = targetW;
    p->rect.x = cx - targetW / 2;
    if (p->rect.x < 0) p->rect.x = 0;
    if (p->rect.x + p->rect.width > SCREEN_W) p->rect.x = SCREEN_W - p->rect.width;
  }
  if (p->expandTimer > 0) p->expandTimer -= dt;
}

/* Reflects a ball off a brick using the minimum-penetration axis of the
   ball's bounding box vs. the brick rect -- a simple, robust approximation
   that reads cleanly and is standard for small arcade breakout clones. */
void ReflectOffBrick(Ball *b, Rectangle rect) {
  float overlapL = (b->pos.x + BALL_RADIUS) - rect.x;
  float overlapR = (rect.x + rect.width) - (b->pos.x - BALL_RADIUS);
  float overlapT = (b->pos.y + BALL_RADIUS) - rect.y;
  float overlapB = (rect.y + rect.height) - (b->pos.y - BALL_RADIUS);
  float minX = fminf(overlapL, overlapR);
  float minY = fminf(overlapT, overlapB);
  if (minX < minY) b->vel.x = -b->vel.x;
  else b->vel.y = -b->vel.y;
}

/* Returns true if the caller should deduct a life (all balls fell out). */
bool UpdateBalls(Paddle *paddle, float dt, int *score, int *comboFx, Sound sfxBrick) {
  bool anyAlive = false;
  for (int i = 0; i < MAX_BALLS; i++) {
    Ball *b = &balls[i];
    if (!b->active) continue;

    if (b->attached) {
      b->pos.x = paddle->rect.x + paddle->rect.width / 2;
      b->pos.y = paddle->rect.y - BALL_RADIUS - 1;
      anyAlive = true;
      continue;
    }

    b->pos = Vector2Add(b->pos, Vector2Scale(b->vel, dt));

    if (b->pos.x - BALL_RADIUS < 0) { b->pos.x = BALL_RADIUS; b->vel.x = -b->vel.x; }
    if (b->pos.x + BALL_RADIUS > SCREEN_W) { b->pos.x = SCREEN_W - BALL_RADIUS; b->vel.x = -b->vel.x; }
    if (b->pos.y - BALL_RADIUS < 0) { b->pos.y = BALL_RADIUS; b->vel.y = -b->vel.y; }

    if (b->vel.y > 0 && CheckCollisionCircleRec(b->pos, BALL_RADIUS, paddle->rect)) {
      float half = paddle->rect.width / 2;
      float offset = ((b->pos.x - (paddle->rect.x + half)) / half);
      if (offset < -1) offset = -1;
      if (offset > 1) offset = 1;
      float angle = offset * MAX_BOUNCE_ANGLE;
      float speed = Vector2Length(b->vel);
      b->vel = (Vector2){speed * sinf(angle), -speed * cosf(angle)};
      b->pos.y = paddle->rect.y - BALL_RADIUS - 0.5f;
      *comboFx = 1;
    }

    for (int r = 0; r < BRICK_ROWS && b->active; r++) {
      for (int c = 0; c < BRICK_COLS; c++) {
        Brick *br = &bricks[r][c];
        if (!br->alive) continue;
        if (!CheckCollisionCircleRec(b->pos, BALL_RADIUS, br->rect)) continue;

        ReflectOffBrick(b, br->rect);
        Vector2 center = {br->rect.x + br->rect.width / 2, br->rect.y + br->rect.height / 2};

        if (!br->unbreakable) {
          br->hp--;
          PlaySound(sfxBrick);
          if (br->hp <= 0) {
            br->alive = false;
            *score += 10 * br->maxHp;
            SpawnParticles(center, kRowColors[r], 8);
            SpawnPowerUp(center);
          }
        }
        break; /* one brick per ball per frame keeps the bounce clean */
      }
    }

    if (b->pos.y - BALL_RADIUS > SCREEN_H) {
      b->active = false;
      continue;
    }
    anyAlive = true;
  }
  return !anyAlive;
}

void UpdatePowerUps(Paddle *paddle, float dt, int *score, Sound sfxPowerUp) {
  for (int i = 0; i < MAX_POWERUPS; i++) {
    PowerUp *pu = &powerups[i];
    if (!pu->active) continue;
    pu->pos.y += POWERUP_SPEED * dt;

    Rectangle puRect = {pu->pos.x - POWERUP_SIZE / 2, pu->pos.y - POWERUP_SIZE / 2,
                         POWERUP_SIZE, POWERUP_SIZE};
    if (CheckCollisionRecs(puRect, paddle->rect)) {
      ApplyPowerUp(pu->kind, paddle, BASE_BALL_SPEED);
      pu->active = false;
      *score += 5;
      PlaySound(sfxPowerUp);
      continue;
    }
    if (pu->pos.y - POWERUP_SIZE > SCREEN_H) pu->active = false;
  }
}

void UpdateParticles(float dt) {
  for (int i = 0; i < MAX_PARTICLES; i++) {
    Particle *pt = &particles[i];
    if (!pt->active) continue;
    pt->vel.y += 300.0f * dt;
    pt->pos = Vector2Add(pt->pos, Vector2Scale(pt->vel, dt));
    pt->life -= dt;
    if (pt->life <= 0) pt->active = false;
  }
}

/* -------------------------------------------------------------------------
   6. DRAWING
   -------------------------------------------------------------------------*/
void DrawBricks(void) {
  for (int r = 0; r < BRICK_ROWS; r++) {
    for (int c = 0; c < BRICK_COLS; c++) {
      Brick *b = &bricks[r][c];
      if (!b->alive) continue;
      if (b->unbreakable) {
        DrawRectangleRec(b->rect, (Color){66, 66, 78, 255});
        DrawRectangleLinesEx(b->rect, 2, (Color){150, 150, 165, 255});
        DrawLineEx((Vector2){b->rect.x + 6, b->rect.y + b->rect.height - 6},
                   (Vector2){b->rect.x + b->rect.width - 6, b->rect.y + 6}, 2,
                   (Color){150, 150, 165, 255});
        continue;
      }
      float dmg = (b->maxHp > 1) ? (float)(b->hp - 1) / (float)(b->maxHp - 1) : 0.0f;
      Color col = ColorLerp(kRowColors[r], WHITE, dmg * 0.5f);
      DrawRectangleRec(b->rect, col);
      DrawRectangleLinesEx(b->rect, 1, Fade(BLACK, 0.35f));
    }
  }
}

void DrawPowerUps(void) {
  for (int i = 0; i < MAX_POWERUPS; i++) {
    if (!powerups[i].active) continue;
    Color c = (powerups[i].kind == POWERUP_EXPAND) ? SKYBLUE : ORANGE;
    const char *label = (powerups[i].kind == POWERUP_EXPAND) ? "E" : "M";
    DrawPoly(powerups[i].pos, 4, POWERUP_SIZE / 1.4f, 45, c);
    DrawPoly(powerups[i].pos, 4, POWERUP_SIZE / 1.4f, 45, Fade(BLACK, 0.3f));
    DrawText(label, (int)powerups[i].pos.x - 3, (int)powerups[i].pos.y - 7, 14, BLACK);
  }
}

void DrawParticles(void) {
  for (int i = 0; i < MAX_PARTICLES; i++) {
    if (!particles[i].active) continue;
    float t = particles[i].life / particles[i].maxLife;
    DrawRectangle((int)particles[i].pos.x, (int)particles[i].pos.y, 3, 3,
                  Fade(particles[i].color, t));
  }
}

void DrawScene(Paddle *paddle) {
  DrawBricks();
  DrawRectangleRounded(paddle->rect, 0.5f, 8, RAYWHITE);
  DrawRectangleRoundedLinesEx(paddle->rect, 0.5f, 8, 2, Fade(BLACK, 0.4f));
  for (int i = 0; i < MAX_BALLS; i++)
    if (balls[i].active) DrawCircleV(balls[i].pos, BALL_RADIUS, RAYWHITE);
  DrawPowerUps();
  DrawParticles();
}

void DrawCenteredText(const char *text, int y, int size, Color color) {
  int w = MeasureText(text, size);
  DrawText(text, SCREEN_W / 2 - w / 2, y, size, color);
}

void DrawHUD(int score, int lives, int level, MusicMode mode) {
  char buf[64];
  snprintf(buf, sizeof(buf), "Score: %d", score);
  DrawText(buf, 16, 12, 20, RAYWHITE);

  snprintf(buf, sizeof(buf), "Level %d / %d", level, MAX_LEVEL);
  DrawCenteredText(buf, 12, 20, RAYWHITE);

  for (int i = 0; i < lives; i++)
    DrawCircle(SCREEN_W - 22 - i * 22, 22, 7, (Color){235, 90, 90, 255});

  const char *modeText = (mode == MODE_CLASSICAL) ? "Mode: Professional [M]" : "Mode: Bangla Fun [M]";
  int w = MeasureText(modeText, 14);
  DrawText(modeText, SCREEN_W - w - 16, 40, 14, Fade(RAYWHITE, 0.75f));
}

/* -------------------------------------------------------------------------
   7. AUDIO -- Dual Music Mode (Part 1 of the brief)
   -------------------------------------------------------------------------*/
Music *TrackForState(GameState state, MusicMode mode, Music *classical[3], Music *bangla[3]) {
  /* classical[] = { menu, gameplay, victory }; bangla[] = { menu, gameplay, gameOver } */
  switch (state) {
    case STATE_MENU:
    case STATE_PAUSED:
      return (mode == MODE_CLASSICAL) ? classical[0] : bangla[0];
    case STATE_PLAYING:
      return (mode == MODE_CLASSICAL) ? classical[1] : bangla[1];
    case STATE_LEVEL_CLEAR:
    case STATE_VICTORY:
      /* Classical mode has a dedicated Victory sting; Bangla mode has no
         victory-specific track in the brief, so it keeps the energetic
         gameplay riff going for the celebration. */
      return (mode == MODE_CLASSICAL) ? classical[2] : bangla[1];
    case STATE_GAME_OVER:
      /* Bangla mode has a dedicated comedic sting; classical mode falls
         back to the calm menu theme since none was specified. */
      return (mode == MODE_CLASSICAL) ? classical[0] : bangla[2];
  }
  return NULL;
}

/* -------------------------------------------------------------------------
   8. MAIN
   -------------------------------------------------------------------------*/
int main(void) {
  InitWindow(SCREEN_W, SCREEN_H, "DX-Ball (Raylib Edition)");
  InitAudioDevice();
  SetTargetFPS(60);
  SetExitKey(KEY_NULL); /* ESC no longer force-quits; window close (X) does */
  SetRandomSeed((unsigned int)time(NULL));

  Music bgmMenuClassical = LoadMusicStream("assets/audio/bgm_classical_menu.wav");
  Music bgmGameplayClassical = LoadMusicStream("assets/audio/bgm_classical_gameplay.wav");
  Music bgmVictoryClassical = LoadMusicStream("assets/audio/bgm_classical_victory.wav");
  Music bgmMenuBangla = LoadMusicStream("assets/audio/bgm_bangla_menu.wav");
  Music bgmGameplayBangla = LoadMusicStream("assets/audio/bgm_bangla_gameplay.wav");
  Music bgmGameOverBangla = LoadMusicStream("assets/audio/bgm_bangla_gameover.wav");

  bgmMenuClassical.looping = true;
  bgmGameplayClassical.looping = true;
  bgmVictoryClassical.looping = false;
  bgmMenuBangla.looping = true;
  bgmGameplayBangla.looping = true;
  bgmGameOverBangla.looping = false;

  Music *classicalTracks[3] = {&bgmMenuClassical, &bgmGameplayClassical, &bgmVictoryClassical};
  Music *banglaTracks[3] = {&bgmMenuBangla, &bgmGameplayBangla, &bgmGameOverBangla};

  Sound sfxPaddle = LoadSound("assets/audio/sfx_paddle.wav");
  Sound sfxBrick = LoadSound("assets/audio/sfx_brick.wav");
  Sound sfxPowerUp = LoadSound("assets/audio/sfx_powerup.wav");
  Sound sfxLifeLost = LoadSound("assets/audio/sfx_life_lost.wav");

  MusicMode musicMode = MODE_CLASSICAL;
  Music *currentTrack = NULL;

  GameState state = STATE_MENU;
  Paddle paddle;
  ResetPaddle(&paddle);
  RackBall(&paddle);

  int score = 0, lives = START_LIVES, level = 1;
  BuildLevel(level);

  while (!WindowShouldClose()) {
    float dt = GetFrameTime();

    if (IsKeyPressed(KEY_M)) musicMode = (musicMode == MODE_CLASSICAL) ? MODE_BANGLA : MODE_CLASSICAL;

    Music *desired = TrackForState(state, musicMode, classicalTracks, banglaTracks);
    if (desired != currentTrack) {
      if (currentTrack && IsMusicValid(*currentTrack)) StopMusicStream(*currentTrack);
      currentTrack = desired;
      if (currentTrack && IsMusicValid(*currentTrack)) {
        SetMusicVolume(*currentTrack, 0.5f);
        PlayMusicStream(*currentTrack);
      }
    }
    if (currentTrack && IsMusicValid(*currentTrack)) UpdateMusicStream(*currentTrack);

    switch (state) {
      case STATE_MENU: {
        if (IsKeyPressed(KEY_ENTER)) {
          score = 0; lives = START_LIVES; level = 1;
          BuildLevel(level);
          ResetPaddle(&paddle);
          RackBall(&paddle);
          state = STATE_PLAYING;
        }
        break;
      }

      case STATE_PLAYING: {
        if (IsKeyPressed(KEY_P)) { state = STATE_PAUSED; break; }

        UpdatePaddle(&paddle, dt);

        if (balls[0].attached && IsKeyPressed(KEY_SPACE)) {
          LaunchBall(&balls[0], BASE_BALL_SPEED + (level - 1) * BALL_SPEED_PER_LEVEL);
        }

        int bounced = 0;
        bool lostAll = UpdateBalls(&paddle, dt, &score, &bounced, sfxBrick);
        if (bounced) PlaySound(sfxPaddle);
        UpdatePowerUps(&paddle, dt, &score, sfxPowerUp);
        UpdateParticles(dt);

        if (lostAll) {
          lives--;
          PlaySound(sfxLifeLost);
          if (lives <= 0) {
            state = STATE_GAME_OVER;
          } else {
            ResetPaddle(&paddle);
            RackBall(&paddle);
          }
        } else if (CountBreakableAlive() == 0) {
          if (level >= MAX_LEVEL) {
            state = STATE_VICTORY;
          } else {
            level++;
            BuildLevel(level);
            ResetPaddle(&paddle);
            RackBall(&paddle);
            state = STATE_LEVEL_CLEAR;
          }
        }
        break;
      }

      case STATE_PAUSED: {
        if (IsKeyPressed(KEY_P)) state = STATE_PLAYING;
        break;
      }

      case STATE_LEVEL_CLEAR: {
        if (IsKeyPressed(KEY_ENTER)) state = STATE_PLAYING;
        break;
      }

      case STATE_GAME_OVER:
      case STATE_VICTORY: {
        if (IsKeyPressed(KEY_ENTER)) state = STATE_MENU;
        break;
      }
    }

    /* ---- Draw ---- */
    BeginDrawing();
    ClearBackground((Color){18, 18, 28, 255});

    if (state == STATE_MENU) {
      DrawCenteredText("DX-BALL", 190, 56, RAYWHITE);
      DrawCenteredText("A Raylib Term Project", 252, 20, GRAY);
      DrawCenteredText("Press ENTER to Start", 340, 24, (Color){124, 196, 116, 255});
      DrawCenteredText("LEFT/RIGHT: Move   SPACE: Launch   P: Pause   M: Toggle Music", 400, 16, GRAY);
      const char *modeText = (musicMode == MODE_CLASSICAL) ? "Mode: Professional [M]" : "Mode: Bangla Fun [M]";
      DrawCenteredText(modeText, 440, 18, SKYBLUE);
    } else if (state == STATE_PLAYING || state == STATE_PAUSED) {
      DrawScene(&paddle);
      DrawHUD(score, lives, level, musicMode);
      if (state == STATE_PAUSED) {
        DrawRectangle(0, 0, SCREEN_W, SCREEN_H, Fade(BLACK, 0.55f));
        DrawCenteredText("PAUSED", 260, 44, RAYWHITE);
        DrawCenteredText("Press P to Resume", 320, 20, GRAY);
      }
    } else if (state == STATE_LEVEL_CLEAR) {
      DrawScene(&paddle);
      DrawHUD(score, lives, level, musicMode);
      DrawRectangle(0, 0, SCREEN_W, SCREEN_H, Fade(BLACK, 0.55f));
      char buf[48];
      snprintf(buf, sizeof(buf), "Level %d Clear!", level - 1);
      DrawCenteredText(buf, 260, 40, (Color){124, 196, 116, 255});
      DrawCenteredText("Press ENTER for the Next Level", 320, 20, RAYWHITE);
    } else if (state == STATE_GAME_OVER) {
      DrawCenteredText("GAME OVER", 240, 48, (Color){235, 90, 90, 255});
      char buf[48];
      snprintf(buf, sizeof(buf), "Final Score: %d", score);
      DrawCenteredText(buf, 310, 24, RAYWHITE);
      DrawCenteredText("Press ENTER to Return to Menu", 360, 18, GRAY);
    } else if (state == STATE_VICTORY) {
      DrawCenteredText("YOU WIN!", 220, 48, (Color){252, 191, 73, 255});
      DrawCenteredText("All levels cleared -- great run!", 280, 22, RAYWHITE);
      char buf[48];
      snprintf(buf, sizeof(buf), "Final Score: %d", score);
      DrawCenteredText(buf, 320, 24, RAYWHITE);
      DrawCenteredText("Press ENTER to Return to Menu", 370, 18, GRAY);
    }

    EndDrawing();
  }

  UnloadMusicStream(bgmMenuClassical);
  UnloadMusicStream(bgmGameplayClassical);
  UnloadMusicStream(bgmVictoryClassical);
  UnloadMusicStream(bgmMenuBangla);
  UnloadMusicStream(bgmGameplayBangla);
  UnloadMusicStream(bgmGameOverBangla);
  UnloadSound(sfxPaddle);
  UnloadSound(sfxBrick);
  UnloadSound(sfxPowerUp);
  UnloadSound(sfxLifeLost);

  CloseAudioDevice();
  CloseWindow();
  return 0;
}
