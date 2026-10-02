#include "raylib.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdbool.h>
#include <math.h>
#include <ctype.h>

#define SCREEN_WIDTH 1280
#define SCREEN_HEIGHT 720

#define ROWS 15
#define COLS 15
#define MAX_LEVELS 3
#define BRICK_TOP_OFFSET 100
#define BRICK_HEIGHT 24
#define BRICK_GAP 4

#define MAX_HIGHSCORES 5
#define MAX_NAME_LEN 25
#define HIGHSCORE_FILE "highscores.txt"

#define MAX_POWERUPS 10
#define MAX_LASERS 20
#define MAX_LIVES 5

#define COLOR_OFFWHITE_BG (Color){245, 242, 235, 255}
#define COLOR_BRICKWALL_BG (Color){18, 20, 26, 255}
#define COLOR_BRICK_BORDER (Color){40, 40, 40, 255}
#define COLOR_PANEL (Color){0, 0, 0, 150}
#define COLOR_PANEL_RECT (Rectangle){190, 60, 900, 600}

#define COLOR_PADDLE_CYAN      (Color){0, 220, 255, 255}       
#define COLOR_BRICK_UNBREAK    (Color){70, 80, 95, 255}     
#define COLOR_BRICK_GREY_BREAK (Color){190, 195, 200, 255}     

#define COLOR_BRICK_ORANGE     (Color){255, 140, 0, 255}
#define COLOR_BRICK_GOLD_YEL   (Color){255, 200, 0, 255}      
#define COLOR_BRICK_SOFT_YEL   (Color){255, 240, 130, 255}    
#define COLOR_BRICK_GREEN      (Color){46, 204, 113, 255}
#define COLOR_BRICK_PURPLE     (Color){186, 85, 211, 255}
#define COLOR_BRICK_RED        (Color){230, 41, 55, 255}
#define COLOR_BRICK_BLUE       (Color){30, 144, 255, 255}

typedef enum GameState
{
    STATE_MENU = 0,
    STATE_DIFFICULTY,
    STATE_NAME_INPUT,
    STATE_GAMEPLAY,
    STATE_LEVEL_CLEAR,
    STATE_GAME_OVER,
    STATE_WIN,
    STATE_HIGH_SCORES,
    STATE_HOW_TO_PLAY,
    STATE_CREDITS
} GameState;

typedef enum Difficulty
{
    DIFF_EASY = 0,
    DIFF_MEDIUM,
    DIFF_HARD
} Difficulty;

Difficulty currentDifficulty = DIFF_MEDIUM;
int difficultySelection = 1;

typedef enum PowerUpType
{
    POWERUP_EXPAND_PADDLE = 0,
    POWERUP_EXTRA_LIFE,
    POWERUP_SLOW_BALL,
    POWERUP_LASER,
    POWERUP_ANTI_LIFE,
    POWERUP_BONUS_SCORE,
    POWERUP_BOMB,
    POWERUP_TYPE_COUNT
} PowerUpType;

typedef struct Paddle
{
    Rectangle rect;
    float speed;
    Color color;
    bool hasLaser;
} Paddle;

typedef struct Ball
{
    Vector2 pos;
    Vector2 speed;
    float radius;
    bool active;
    Color color;
} Ball;

typedef struct Brick
{
    Rectangle rect;
    bool active;
    Color color;
    bool unbreakable;
    int hitPoints;
    int maxHits;
} Brick;

typedef struct PowerUp
{
    Vector2 pos;
    Vector2 speed;
    PowerUpType type;
    bool active;
    float radius;
} PowerUp;

typedef struct Laser
{
    Rectangle rect;
    Vector2 speed;
    bool active;
} Laser;

typedef struct Bomb
{
    Vector2 pos;
    bool active;
    float speed;
} Bomb;

typedef struct HighScoreEntry
{
    char name[MAX_NAME_LEN + 1];
    int score;
} HighScoreEntry;

GameState currentState = STATE_MENU;
bool exitRequested = false;

int menuSelection = 0;
int endSelection = 0;

bool isPaused = false;
int pauseOption = 0;

Paddle paddle;
Ball ball;
Brick bricks[ROWS][COLS];
PowerUp powerUps[MAX_POWERUPS];
Laser lasers[MAX_LASERS];
Bomb bomb;

int score = 0;
int lives = 3;
int currentLevel = 1;
int lastMilestone = 0;

char playerName[MAX_NAME_LEN + 1] = {0};
int nameLetterCount = 0;

HighScoreEntry highScores[MAX_HIGHSCORES];
int highScoreCount = 0;

Texture2D menuBgTexture;
Texture2D bgLevel1;
Texture2D bgLevel2;
Texture2D bgLevel3;

Texture2D ballTexture;
Sound sfxBrickBreak;
Sound sfxBrickDamaged;
Sound sfxPaddleHit;
Sound sfxWallHit;
Sound sfxLose;
Sound sfxGameOver;
Sound sfxWinFanfare;
Sound sfxConfirm;
Sound sfxLevelClear;
Sound sfxMilestone;

Music bgMusic;
bool bgMusicLoaded = false;
Music gameplayMusic;
bool gameplayMusicLoaded = false;

void InitGame(void);
void ResetPaddleAndBall(void);
void ClearPowerUpsAndLasers(void);
void LoadLevel(int level);

int AssignBrickDurability(int level);
float GetLevelBallSpeed(int level);
void AwardScore(int amount);

float CalculateBounceAngle(Vector2 ballPos, Rectangle paddleRect);
void SetBallVelocityFromAngle(float angleDeg, float speedMag);
void ReflectBallX(void);
void ReflectBallY(void);
void ResolveBallBrickCollision(Rectangle brickRect);
void BounceOffUnbreakable(Rectangle brickRect);
bool DamageBrick(int row, int col);

PowerUpType ChoosePowerUpTypeForLevel(int level);
void SpawnPowerUp(Vector2 pos);
void UpdatePowerUps(void);
void DrawPowerUps(void);
void ApplyPowerUpEffect(PowerUpType type);

void ShootLaser(void);
void UpdateLasers(void);
void DrawLasers(void);

void ActivateBomb(void);
void UpdateBomb(void);
void DrawBomb(void);

void LoadBackgrounds(void);
void UnloadBackgrounds(void);
void InitAudio(void);
void ToggleAudio(void);
void UnloadAudioAssets(void);
void PlayConfirmSfx(void);
void UpdateBackgroundMusic(void);

void LoadHighScores(void);
void SaveHighScores(void);
void AddHighScore(const char *name, int newScore);
int CompareHighScoresDesc(const void *a, const void *b);

void UpdateMenu(void);
void UpdateDifficultyMenu(void);
void DrawDifficultyMenu(void);
void UpdateNameInput(void);
void UpdateGameplay(void);
void CheckCollisions(void);
void UpdateLevelClear(void);
void UpdateGameOver(void);
void UpdateWin(void);
void UpdateHighScores(void);
void UpdateHowToPlay(void);
void UpdateCredits(void);
bool UpdateEndScreenNav(void);

void DrawMenu(void);
void DrawNameInput(void);
void DrawGameplay(void);
void DrawLevelClear(void);
void DrawGameOver(void);
void DrawWin(void);
void DrawHighScores(void);
void DrawHowToPlay(void);
void DrawCredits(void);

void DrawMenuStyleBackground(void);
void DrawGameplayStyleBackground(void);
void DrawEndScreen(const char *title, Color titleColor);
void DrawBrick(const Brick *b);
void DrawHUD(void);

int main(void)
{
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "DX-BALL");
    SetTargetFPS(60);
    srand((unsigned int)time(NULL));

    InitAudioDevice();
    InitAudio();
    LoadBackgrounds();
    LoadHighScores();

    while (!WindowShouldClose() && !exitRequested)
    {
        ToggleAudio();

        switch (currentState)
        {
        case STATE_MENU:
            UpdateMenu();
            break;
        
      case STATE_DIFFICULTY:
        UpdateDifficultyMenu();
         break;

        case STATE_NAME_INPUT:
            UpdateNameInput();
            break;
        case STATE_GAMEPLAY:
            UpdateGameplay();
            break;
        case STATE_LEVEL_CLEAR:
            UpdateLevelClear();
            break;
        case STATE_GAME_OVER:
            UpdateGameOver();
            break;
        case STATE_WIN:
            UpdateWin();
            break;
        case STATE_HIGH_SCORES:
            UpdateHighScores();
            break;
        case STATE_HOW_TO_PLAY:
            UpdateHowToPlay();
            break;
        case STATE_CREDITS:
            UpdateCredits();
            break;
        }

        UpdateBackgroundMusic();

        BeginDrawing();

        switch (currentState)
        {
        case STATE_MENU:
            DrawMenu();
            break;
            case STATE_DIFFICULTY:
             DrawDifficultyMenu();
            break;
        case STATE_NAME_INPUT:
            DrawNameInput();
            break;
        case STATE_GAMEPLAY:
            DrawGameplay();
            break;
        case STATE_LEVEL_CLEAR:
            DrawLevelClear();
            break;
        case STATE_GAME_OVER:
            DrawGameOver();
            break;
        case STATE_WIN:
            DrawWin();
            break;
        case STATE_HIGH_SCORES:
            DrawHighScores();
            break;
        case STATE_HOW_TO_PLAY:
            DrawHowToPlay();
            break;
        case STATE_CREDITS:
            DrawCredits();
            break;
        }

        EndDrawing();
    }

    UnloadAudioAssets();
    UnloadBackgrounds();
    CloseAudioDevice();
    CloseWindow();
    return 0;
}

int AssignBrickDurability(int level)
{
    if (level == 1)
        return 1;
    int roll = rand() % 100;
    if (roll < 5)
        return 3;
    if (roll < 15)
        return 2;
    return 1;
}

float GetLevelBallSpeed(int level)
{
    float baseSpeed = 7.0f;
    if (currentDifficulty == DIFF_EASY) baseSpeed = 5.5f;
    else if (currentDifficulty == DIFF_MEDIUM) baseSpeed = 7.5f;
    else if (currentDifficulty == DIFF_HARD) baseSpeed = 9.5f;

    return baseSpeed + (level - 1) * 1.0f;
}

void UpdateDifficultyMenu(void)
{
    if (IsKeyPressed(KEY_DOWN))
        difficultySelection = (difficultySelection + 1) % 3;
    if (IsKeyPressed(KEY_UP))
        difficultySelection = (difficultySelection + 2) % 3;

    if (IsKeyPressed(KEY_ENTER))
    {
        PlayConfirmSfx();
        currentDifficulty = (Difficulty)difficultySelection;
        
        playerName[0] = '\0';
        nameLetterCount = 0;
        currentState = STATE_NAME_INPUT;
    }

    if (IsKeyPressed(KEY_ESCAPE))
    {
        currentState = STATE_MENU;
    }
}

void DrawDifficultyMenu(void)
{
    DrawMenuStyleBackground();

    DrawText("SELECT DIFFICULTY", SCREEN_WIDTH / 2 - MeasureText("SELECT DIFFICULTY", 40) / 2, 160, 40, GOLD);

    const char *options[3] = {"EASY (Slow Ball)", "MEDIUM (Normal Speed)", "HARD (Fast Ball)"};
    for (int i = 0; i < 3; i++)
    {
        Color c = (i == difficultySelection) ? YELLOW : WHITE;
        int fontSize = (i == difficultySelection) ? 32 : 26;
        DrawText(options[i], SCREEN_WIDTH / 2 - MeasureText(options[i], fontSize) / 2,
                 280 + i * 60, fontSize, c);
    }

    const char *hint = "Use UP / DOWN and press ENTER";
    DrawText(hint, SCREEN_WIDTH / 2 - MeasureText(hint, 18) / 2, 580, 18, YELLOW);
}

void AwardScore(int amount)
{
    score += amount;
    if (score / 100 > lastMilestone)
    {
        lastMilestone = score / 100;
        PlaySound(sfxMilestone);
    }
}

float CalculateBounceAngle(Vector2 ballPos, Rectangle paddleRect)
{
    float relativeX = (ballPos.x - paddleRect.x) / paddleRect.width;
    if (relativeX < 0.0)
        relativeX = 0.0;
    if (relativeX > 1.0)
        relativeX = 1.0;
    return 180-(170.0 - relativeX * 160.0);
}

void SetBallVelocityFromAngle(float angleDeg, float speedMag)
{
    float angleRad = angleDeg * DEG2RAD;
    ball.speed.x = speedMag * cosf(angleRad);
    ball.speed.y = -speedMag * sinf(angleRad);
}

void ReflectBallX(void) { ball.speed.x *= -1; }
void ReflectBallY(void) { ball.speed.y *= -1; }

void ResolveBallBrickCollision(Rectangle brickRect)
{
    float brickCenterX = brickRect.x + brickRect.width / 2.0;
    float brickCenterY = brickRect.y + brickRect.height / 2.0;

    float dx = ball.pos.x - brickCenterX;
    float dy = ball.pos.y - brickCenterY;

    float halfW = brickRect.width / 2.0;
    float halfH = brickRect.height / 2.0;

    float overlapX = (ball.radius + halfW) - fabsf(dx);
    float overlapY = (ball.radius + halfH) - fabsf(dy);

    if (overlapX < overlapY)
    {

        ball.speed.x = -ball.speed.x;
        ball.pos.x += (dx < 0) ? -(overlapX + 0.5) : (overlapX + 0.5);
    }
    else
    {

        ball.speed.y = -ball.speed.y;
        ball.pos.y += (dy < 0) ? -(overlapY + 0.5) : (overlapY + 0.5);
    }
}

void BounceOffUnbreakable(Rectangle brickRect)
{
    ResolveBallBrickCollision(brickRect);
    PlaySound(sfxWallHit);
}

bool DamageBrick(int row, int col)
{
    if (bricks[row][col].unbreakable)
        return false;

    bricks[row][col].hitPoints--;
    if (bricks[row][col].hitPoints <= 0)
    {
        bricks[row][col].active = false;
        return true;
    }
    PlaySound(sfxBrickDamaged);
    return false;
}

PowerUpType ChoosePowerUpTypeForLevel(int level)
{
    if (level == 1)
        return (rand() % 2 == 0) ? POWERUP_EXPAND_PADDLE : POWERUP_EXTRA_LIFE;

    if (level == 2)
        return (PowerUpType)(rand() % 6);
    return (PowerUpType)(rand() % 7);
}

bool isAudioMuted = false;

void ToggleAudio(void)
{
    if (IsKeyPressed(KEY_M))
    {
        isAudioMuted = !isAudioMuted;

        if (isAudioMuted)
        {
            SetMasterVolume(0.0f); 
        }
        else
        {
            SetMasterVolume(1.0f);
        }
    }
}


void SpawnPowerUp(Vector2 pos)
{
    if (rand() % 100 < 20)
    {
        for (int i = 0; i < MAX_POWERUPS; i++)
        {
            if (!powerUps[i].active)
            {
                powerUps[i].pos = pos;
                powerUps[i].speed = (Vector2){0.0, 3.0};
                powerUps[i].type = ChoosePowerUpTypeForLevel(currentLevel);
                powerUps[i].active = true;
                powerUps[i].radius = 12.0;
                break;
            }
        }
    }
}

void ApplyPowerUpEffect(PowerUpType type)
{
    switch (type)
    {
    case POWERUP_EXPAND_PADDLE:
        paddle.rect.width += 30;
        if (paddle.rect.width > 260)
            paddle.rect.width = 260;
        PlaySound(sfxConfirm);
        break;
    case POWERUP_EXTRA_LIFE:
        lives++;
        if (lives > MAX_LIVES)
            lives = MAX_LIVES;
        PlaySound(sfxConfirm);
        break;
    case POWERUP_SLOW_BALL:
        ball.speed.x *= 0.8;
        ball.speed.y *= 0.8;
        PlaySound(sfxConfirm);
        break;
    case POWERUP_LASER:
        paddle.hasLaser = true;
        PlaySound(sfxConfirm);
        break;
    case POWERUP_ANTI_LIFE:
        lives--;
        PlaySound(sfxLose);
        if (lives <= 0)
        {
            AddHighScore(playerName, score);
            endSelection = 0;
            currentState = STATE_GAME_OVER;
            PlaySound(sfxGameOver);
        }
        break;
    case POWERUP_BONUS_SCORE:
        AwardScore(50);
        PlaySound(sfxConfirm);
        break;
    case POWERUP_BOMB:
        ActivateBomb();
        break;
    default:
        break;
    }
}

void UpdatePowerUps(void)
{
    for (int i = 0; i < MAX_POWERUPS; i++)
    {
        if (!powerUps[i].active)
            continue;
        powerUps[i].pos.y += powerUps[i].speed.y;

        if (CheckCollisionCircleRec(powerUps[i].pos, powerUps[i].radius, paddle.rect))
        {
            powerUps[i].active = false;
            ApplyPowerUpEffect(powerUps[i].type);
        }
        if (powerUps[i].pos.y > SCREEN_HEIGHT)
            powerUps[i].active = false;
    }
}

void DrawPowerUps(void)
{
    for (int i = 0; i < MAX_POWERUPS; i++)
    {
        if (!powerUps[i].active)
            continue;

        Color c;
        const char *label;
        switch (powerUps[i].type)
        {
        case POWERUP_EXPAND_PADDLE:
            c = BLUE;
            label = "W";
            break;
        case POWERUP_EXTRA_LIFE:
            c = GREEN;
            label = "+";
            break;
        case POWERUP_SLOW_BALL:
            c = ORANGE;
            label = "S";
            break;
        case POWERUP_LASER:
            c = PURPLE;
            label = "L";
            break;
        case POWERUP_ANTI_LIFE:
            c = RED;
            label = "X";
            break;
        case POWERUP_BONUS_SCORE:
            c = GOLD;
            label = "$";
            break;
        case POWERUP_BOMB:
            c = MAROON;
            label = "B";
            break;
        default:
            c = GRAY;
            label = "?";
            break;
        }
        DrawCircleV(powerUps[i].pos, powerUps[i].radius, c);
        DrawText(label, (int)powerUps[i].pos.x - 4,
                 (int)powerUps[i].pos.y - 6, 14, WHITE);
    }
}

void ShootLaser(void)
{
    int spawned = 0;
    for (int i = 0; i < MAX_LASERS && spawned < 2; i++)
    {
        if (!lasers[i].active)
        {
            float offsetX = (spawned == 0) ? 5.0 : paddle.rect.width - 9.0;
            lasers[i].rect = (Rectangle){paddle.rect.x + offsetX, paddle.rect.y - 12, 4, 12};
            lasers[i].speed = (Vector2){0.0, -10.0};
            lasers[i].active = true;
            spawned++;
        }
    }
    PlaySound(sfxPaddleHit);
}

void UpdateLasers(void)
{
    for (int i = 0; i < MAX_LASERS; i++)
    {
        if (!lasers[i].active)
            continue;

        lasers[i].rect.y += lasers[i].speed.y;
        if (lasers[i].rect.y < 0)
        {
            lasers[i].active = false;
            continue;
        }

        for (int r = 0; r < ROWS; r++)
        {
            bool hit = false;
            for (int c = 0; c < COLS; c++)
            {
                if (!bricks[r][c].active)
                    continue;
                if (!CheckCollisionRecs(lasers[i].rect, bricks[r][c].rect))
                    continue;

                if (bricks[r][c].unbreakable)
                {
                    lasers[i].active = false;
                    PlaySound(sfxWallHit);
                }
                else
                {
                    bool destroyed = DamageBrick(r, c);
                    lasers[i].active = false;
                    if (destroyed)
                    {
                        AwardScore(currentLevel * 10);
                        PlaySound(sfxBrickBreak);
                        Vector2 p = {bricks[r][c].rect.x + bricks[r][c].rect.width / 2,
                                     bricks[r][c].rect.y};
                        SpawnPowerUp(p);
                    }
                }
                hit = true;
                break;
            }
            if (hit)
                break;
        }
    }
}

void DrawLasers(void)
{
    for (int i = 0; i < MAX_LASERS; i++)
        if (lasers[i].active)
            DrawRectangleRec(lasers[i].rect, RED);
}

void ActivateBomb(void)
{
    bomb.pos = (Vector2){paddle.rect.x + paddle.rect.width / 2, paddle.rect.y - 12};
    bomb.speed = 12.0;
    bomb.active = true;
    PlaySound(sfxConfirm);
}

void UpdateBomb(void)
{
    if (!bomb.active)
        return;

    bomb.pos.y -= bomb.speed;

    if (bomb.pos.y < 0)
    {
        bomb.active = false;
        return;
    }

    for (int r = 0; r < ROWS; r++)
    {
        for (int c = 0; c < COLS; c++)
        {
            if (!bricks[r][c].active)
                continue;
            if (!CheckCollisionCircleRec(bomb.pos, 6.0, bricks[r][c].rect))
                continue;

            if (bricks[r][c].unbreakable)
            {
                bomb.active = false;
                return;
            }

            bricks[r][c].active = false;
            AwardScore(currentLevel * 10);
            PlaySound(sfxBrickBreak);
            Vector2 p = {bricks[r][c].rect.x + bricks[r][c].rect.width / 2,
                         bricks[r][c].rect.y};
            SpawnPowerUp(p);
        }
    }
}

void DrawBomb(void)
{
    if (!bomb.active)
        return;
    DrawCircleV(bomb.pos, 8, MAROON);
    DrawCircleV(bomb.pos, 4, ORANGE);
    DrawText("B", (int)bomb.pos.x - 4, (int)bomb.pos.y - 7, 12, WHITE);
}
/*
void LoadBackgrounds(void)
{

    menuBgTexture = LoadTexture("resources/menu_bg.png");
    bgLevel1 = LoadTexture("resources/bg_level1.png");
    bgLevel2 = LoadTexture("resources/bg_level2.png");
    bgLevel3 = LoadTexture("resources/bg_level3.png");
    ballTexture = LoadTexture("resources/ball_r9.png");
}
    */



void LoadBackgrounds(void)
{

    menuBgTexture = LoadTexture("resources/menu_bg.png");
  bgLevel1 = LoadTexture("resources/bg1.png");
bgLevel2 = LoadTexture("resources/bg2.png");
bgLevel3 = LoadTexture("resources/bg3.png");
    ballTexture = LoadTexture("resources/ball_r9.png");
}


void UnloadBackgrounds(void)
{
    UnloadTexture(menuBgTexture);
    UnloadTexture(bgLevel1);
    UnloadTexture(bgLevel2);
    UnloadTexture(bgLevel3);
    UnloadTexture(ballTexture);
}

void InitAudio(void)
{
    sfxBrickBreak = LoadSound("resources/sfx_brick_break.wav");
    sfxBrickDamaged = LoadSound("resources/sfx_brick_damaged.wav");
    sfxPaddleHit = LoadSound("resources/sfx_paddle_hit.wav");
    sfxWallHit = LoadSound("resources/sfx_wall_hit.wav");
    sfxLose = LoadSound("resources/sfx_lose.wav");
    sfxGameOver = LoadSound("resources/sfx_game_over.wav");
    sfxWinFanfare = LoadSound("resources/sfx_win_fanfare.wav");
    sfxConfirm = LoadSound("resources/sfx_confirm.wav");
    sfxLevelClear = LoadSound("resources/sfx_level_clear.wav");
    sfxMilestone = LoadSound("resources/sfx_milestone.wav");

    bgMusic = LoadMusicStream("resources/soothing_bg.mp3");
    if (bgMusic.frameCount > 0)
    {
        bgMusic.looping = true;
        SetMusicVolume(bgMusic, 0.5);
        bgMusicLoaded = true;
    }
    gameplayMusic = LoadMusicStream("resources/menu_bgm.mp3");
    if (gameplayMusic.frameCount > 0)
    {
        gameplayMusic.looping = true;
        SetMusicVolume(gameplayMusic, 0.2);
        gameplayMusicLoaded = true;
    }

    SetSoundVolume(sfxBrickBreak, 0.8);
    SetSoundVolume(sfxBrickDamaged, 0.9);
    SetSoundVolume(sfxPaddleHit, 0.8);
    SetSoundVolume(sfxWallHit, 0.7);
    SetSoundVolume(sfxLose, 0.9);
    SetSoundVolume(sfxGameOver, 0.9);
    SetSoundVolume(sfxWinFanfare, 0.9);
    SetSoundVolume(sfxConfirm, 0.7);
    SetSoundVolume(sfxLevelClear, 0.8);
    SetSoundVolume(sfxMilestone, 0.9);
}

void UnloadAudioAssets(void)
{
    UnloadSound(sfxBrickBreak);
    UnloadSound(sfxBrickDamaged);
    UnloadSound(sfxPaddleHit);
    UnloadSound(sfxWallHit);
    UnloadSound(sfxLose);
    UnloadSound(sfxGameOver);
    UnloadSound(sfxWinFanfare);
    UnloadSound(sfxConfirm);
    UnloadSound(sfxLevelClear);
    UnloadSound(sfxMilestone);
    if (gameplayMusicLoaded)
        UnloadMusicStream(gameplayMusic);
    if (bgMusicLoaded)
        UnloadMusicStream(bgMusic);
}

void PlayConfirmSfx(void) { PlaySound(sfxConfirm); }

void UpdateBackgroundMusic(void)
{
    if (bgMusicLoaded)
    {
        bool wantMenuMusic = (currentState == STATE_MENU) ||
                             (currentState == STATE_NAME_INPUT) ||
                             (currentState == STATE_HOW_TO_PLAY) ||
                             (currentState == STATE_CREDITS) ||
                             (currentState == STATE_HIGH_SCORES);
        if (wantMenuMusic)
        {
            if (!IsMusicStreamPlaying(bgMusic))
                PlayMusicStream(bgMusic);
            UpdateMusicStream(bgMusic);
        }
        else if (IsMusicStreamPlaying(bgMusic))
            StopMusicStream(bgMusic);
    }

    if (gameplayMusicLoaded)
    {
        bool wantGameplayMusic = (currentState == STATE_GAMEPLAY);
        if (wantGameplayMusic)
        {
            if (!IsMusicStreamPlaying(gameplayMusic))
                PlayMusicStream(gameplayMusic);
            UpdateMusicStream(gameplayMusic);
        }
        else if (IsMusicStreamPlaying(gameplayMusic))
            StopMusicStream(gameplayMusic);
    }
}

int CompareHighScoresDesc(const void *a, const void *b)
{
    const HighScoreEntry *ea = (const HighScoreEntry *)a;
    const HighScoreEntry *eb = (const HighScoreEntry *)b;
    return eb->score - ea->score;
}

void LoadHighScores(void)
{
    highScoreCount = 0;
    FILE *f = fopen(HIGHSCORE_FILE, "r");
    if (f == NULL)
        return;

    char nameBuf[MAX_NAME_LEN + 1];
    int scoreBuf;

    while (highScoreCount < MAX_HIGHSCORES &&
           fscanf(f, "%25s %d", nameBuf, &scoreBuf) == 2)
    {
        strncpy(highScores[highScoreCount].name, nameBuf, MAX_NAME_LEN);
        highScores[highScoreCount].name[MAX_NAME_LEN] = '\0';
        highScores[highScoreCount].score = scoreBuf;
        highScoreCount++;
    }

    fclose(f);
    qsort(highScores, highScoreCount, sizeof(HighScoreEntry), CompareHighScoresDesc);
}

void SaveHighScores(void)
{
    FILE *f = fopen(HIGHSCORE_FILE, "w");
    if (f == NULL)
        return;
    for (int i = 0; i < highScoreCount; i++)
        fprintf(f, "%s %d\n", highScores[i].name, highScores[i].score);
    fclose(f);
}

void AddHighScore(const char *name, int newScore)
{
    if (highScoreCount < MAX_HIGHSCORES)
    {
        strncpy(highScores[highScoreCount].name, name, MAX_NAME_LEN);
        highScores[highScoreCount].name[MAX_NAME_LEN] = '\0';
        highScores[highScoreCount].score = newScore;
        highScoreCount++;
    }
    else
    {
        int lowestIdx = 0;
        for (int i = 1; i < highScoreCount; i++)
            if (highScores[i].score < highScores[lowestIdx].score)
                lowestIdx = i;

        if (newScore > highScores[lowestIdx].score)
        {
            strncpy(highScores[lowestIdx].name, name, MAX_NAME_LEN);
            highScores[lowestIdx].name[MAX_NAME_LEN] = '\0';
            highScores[lowestIdx].score = newScore;
        }
    }

    qsort(highScores, highScoreCount, sizeof(HighScoreEntry), CompareHighScoresDesc);
    SaveHighScores();
}

void ClearPowerUpsAndLasers(void)
{
    for (int i = 0; i < MAX_POWERUPS; i++)
        powerUps[i].active = false;
    for (int i = 0; i < MAX_LASERS; i++)
        lasers[i].active = false;
    bomb.active = false;
}

void InitGame(void)
{
    score = 0;
    lives = 3;
    currentLevel = 1;
    lastMilestone = 0;
    isPaused = false;
    endSelection = 0;

    ClearPowerUpsAndLasers();
    ResetPaddleAndBall();
    LoadLevel(currentLevel);
}

void ResetPaddleAndBall(void)
{
    paddle.rect = (Rectangle){SCREEN_WIDTH / 2.0 - 60, SCREEN_HEIGHT - 50, 120, 18};
    paddle.speed = 8.0;
    paddle.color = COLOR_PADDLE_CYAN;
    paddle.hasLaser = false;

    ball.pos = (Vector2){paddle.rect.x + paddle.rect.width / 2.0, paddle.rect.y - 11};
    ball.speed = (Vector2){0.0, 0.0};
    ball.radius = 9.0;
    ball.active = false;
    ball.color = MAROON;
}

void LoadLevel(int level)
{
    int brickWidth = (SCREEN_WIDTH - BRICK_GAP) / COLS;
    int levelMap[ROWS][COLS] = {0};

    if (level == 1)
    {
        int map1[ROWS][COLS] = {
            {9, 6, 9, 7, 9, 8, 9, 3, 9, 5, 9, 6, 9, 7, 9},
            {3, 1, 3, 1, 4, 1, 3, 1, 4, 1, 3, 1, 4, 1, 5},
            {0, 3, 5, 3, 5, 3, 5, 3, 5, 3, 5, 3, 5, 3, 0},
            {0, 0, 4, 2, 4, 2, 4, 2, 4, 2, 4, 2, 4, 0, 0},
            {0, 0, 0, 6, 7, 6, 7, 6, 7, 6, 7, 6, 0, 0, 0},
            {0, 0, 0, 0, 5, 2, 5, 2, 5, 2, 5, 0, 0, 0, 0},
            {0, 0, 0, 0, 0, 4, 3, 4, 3, 4, 0, 0, 0, 0, 0},
            {0, 0, 0, 0, 0, 0, 2, 1, 2, 0, 0, 0, 0, 0, 0},
            {0, 0, 0, 0, 0, 0, 0, 7, 0, 0, 0, 0, 0, 0, 0},
            {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
            {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
            {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}
        };
        memcpy(levelMap, map1, sizeof(map1));
    }
    else if (level == 2)
    {
        int map2[ROWS][COLS] = {
            {1, 8, 0, 0, 5, 8, 0, 0, 4, 8, 0, 0, 6, 8, 6},
            {0, 1, 8, 0, 0, 5, 8, 0, 0, 4, 8, 0, 0, 6, 8},
            {0, 0, 1, 8, 0, 0, 5, 8, 0, 0, 4, 8, 0, 0, 6},
            {8, 0, 0, 1, 8, 0, 0, 5, 8, 0, 0, 4, 8, 0, 0},
            {6, 8, 0, 0, 1, 8, 2, 3, 2, 8, 0, 0, 4, 8, 0},
            {9, 9, 9, 9, 9, 9, 3, 4, 6, 9, 9, 9, 9, 9, 9},
            {8, 6, 8, 0, 0, 2, 8, 0, 8, 2, 8, 0, 0, 4, 8},
            {3, 4, 3, 5, 3, 5, 4, 8, 3, 5, 4, 5, 3, 4, 3},
            {0, 0, 0, 0, 3, 4, 5, 3, 5, 4, 3, 0, 0, 0, 0},
            {0, 0, 0, 0, 0, 3, 4, 5, 4, 3, 0, 0, 0, 0, 0},
            {0, 0, 0, 0, 0, 0, 3, 4, 3, 0, 0, 0, 0, 0, 0},
            {0, 0, 0, 0, 0, 0, 0, 3, 0, 0, 0, 0, 0, 0, 0},
            {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
            {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}
        };
        memcpy(levelMap, map2, sizeof(map2));
    }
    else if (level == 3)
    {
        int map3[ROWS][COLS] = {
            {7, 1, 2, 3, 7, 8, 7, 8, 7, 8, 7, 3, 2, 1, 7},
            {8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 8},
            {1, 0, 1, 3, 1, 0, 4, 3, 1, 0, 4, 3, 2, 0, 1},
            {7, 0, 2, 0, 0, 0, 3, 0, 0, 0, 4, 0, 0, 0, 7},
            {8, 0, 3, 0, 0, 0, 2, 3, 1, 0, 2, 4, 3, 0, 8},
            {3, 0, 4, 0, 0, 0, 0, 0, 3, 0, 4, 0, 0, 0, 2},
            {7, 0, 5, 1, 5, 0, 3, 1, 2, 0, 5, 4, 1, 0, 7},
            {4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 8},
            {5, 0, 2, 1, 0, 0, 5, 1, 5, 0, 6, 3, 6, 0, 6},
            {7, 0, 0, 2, 0, 0, 1, 0, 7, 0, 0, 0, 6, 0, 7},
            {8, 0, 0, 5, 0, 0, 2, 0, 3, 0, 0, 6, 1, 0, 8},
            {6, 0, 0, 4, 0, 0, 3, 0, 1, 0, 5, 0, 0, 0, 5},
            {7, 0, 2, 3, 2, 0, 5, 4, 5, 0, 6, 4, 6, 0, 7},
            {8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 8},
            {7, 2, 1, 4, 9, 9, 9, 2, 3, 9, 9, 9, 4, 2, 7}
        };
        memcpy(levelMap, map3, sizeof(map3));
    }

    for (int i = 0; i < ROWS; i++)
    {
        for (int j = 0; j < COLS; j++)
        {
            Brick *brick = &bricks[i][j];
            int type = levelMap[i][j];

            brick->rect = (Rectangle){
                j * brickWidth + BRICK_GAP,
                i * (BRICK_HEIGHT + BRICK_GAP) + BRICK_TOP_OFFSET,
                brickWidth - BRICK_GAP,
                BRICK_HEIGHT
            };

            brick->active = (type != 0);

            if (type == 9)
            {
                brick->color = COLOR_BRICK_UNBREAK;
                brick->unbreakable = true;
                brick->maxHits = 1;
                brick->hitPoints = 1;
                continue;
            }

            brick->unbreakable = false;

            switch (type)
            {
            case 1:
                brick->color = COLOR_BRICK_ORANGE;
                break;
            case 2:
                brick->color = COLOR_BRICK_GOLD_YEL;
                break;
            case 3:
                brick->color = COLOR_BRICK_GREEN;
                break;
            case 4:
                brick->color = COLOR_BRICK_PURPLE;
                break;
            case 5:
                brick->color = COLOR_BRICK_RED;
                break;
            case 6:
                brick->color = COLOR_BRICK_BLUE;
                break;
            case 7:
                brick->color = COLOR_BRICK_GREY_BREAK;
                break;
            case 8:
                brick->color = COLOR_BRICK_SOFT_YEL;
                break;
            default:
                brick->color = GRAY;
                break;
            }

            brick->maxHits = AssignBrickDurability(level);
            brick->hitPoints = brick->maxHits;
        }
    }
}



void UpdateMenu(void)
{
    if (IsKeyPressed(KEY_DOWN))
        menuSelection = (menuSelection + 1) % 5;
    if (IsKeyPressed(KEY_UP))
        menuSelection = (menuSelection + 4) % 5;

    if (IsKeyPressed(KEY_ENTER))
    {
        PlayConfirmSfx();
        if (menuSelection == 0)
        {
            difficultySelection = 1;
            currentState = STATE_DIFFICULTY;
        }
        else if (menuSelection == 1)
            currentState = STATE_HOW_TO_PLAY;
        else if (menuSelection == 2)
            currentState = STATE_HIGH_SCORES;
        else if (menuSelection == 3)
            currentState = STATE_CREDITS;
        else
            exitRequested = true;
    }
}

void DrawMenuStyleBackground(void)
{
    ClearBackground(COLOR_BRICKWALL_BG);
    if (menuBgTexture.id != 0)
        DrawTexture(menuBgTexture, 0, 0, WHITE);
    DrawRectangleRec(COLOR_PANEL_RECT, COLOR_PANEL);
}
/*
void DrawGameplayStyleBackground(void)
{
    ClearBackground(COLOR_OFFWHITE_BG);
    if (currentLevel == 1)
        DrawTexture(bgLevel1, 0, 0, WHITE);
    else if (currentLevel == 2)
        DrawTexture(bgLevel2, 0, 0, WHITE);
    else if (currentLevel == 3)
        DrawTexture(bgLevel3, 0, 0, WHITE);
}
        */

void DrawGameplayStyleBackground(void)
{
    ClearBackground(COLOR_OFFWHITE_BG);


    Rectangle destRec = { 0.0f, 0.0f, (float)GetScreenWidth(), (float)GetScreenHeight() };
    Vector2 origin = { 0.0f, 0.0f };

    if (currentLevel == 1)
    {
        Rectangle sourceRec = { 0.0f, 0.0f, (float)bgLevel1.width, (float)bgLevel1.height };
        DrawTexturePro(bgLevel1, sourceRec, destRec, origin, 0.0f, WHITE);
    }
    else if (currentLevel == 2)
    {
        Rectangle sourceRec = { 0.0f, 0.0f, (float)bgLevel2.width, (float)bgLevel2.height };
        DrawTexturePro(bgLevel2, sourceRec, destRec, origin, 0.0f, WHITE);
    }
    else if (currentLevel == 3)
    {
        Rectangle sourceRec = { 0.0f, 0.0f, (float)bgLevel3.width, (float)bgLevel3.height };
        DrawTexturePro(bgLevel3, sourceRec, destRec, origin, 0.0f, WHITE);
    }
}

void DrawMenu(void)
{
    DrawMenuStyleBackground();

    DrawText("CSE 102 PROJECT", SCREEN_WIDTH / 2 - MeasureText("CSE 102 PROJECT", 30) / 2, 90, 30, GOLD);
    DrawText("DX-BALL", SCREEN_WIDTH / 2 - MeasureText("DX-BALL", 80) / 2, 130, 90, WHITE);
    //DrawText("whille(!gameOver) keep_bouncing();",
            // SCREEN_WIDTH / 2 - MeasureText("whille(!gameOver) keep_bouncing();", 30) / 2, 230, 30, LIME);

    //int topScore = (highScoreCount > 0) ? highScores[0].score : 0;
   // const char *highLine = TextFormat("HIGH SCORE: %d", topScore);
    //DrawText(highLine, SCREEN_WIDTH / 2 - MeasureText(highLine, 22) / 2, 270, 22, ORANGE

//);

    const char *options[5] = {"Play", "How To Play", "Leaderboard", "Credits", "Exit"};
    for (int i = 0; i < 5; i++)
    {
        Color c = (i == menuSelection) ? YELLOW : WHITE;
        int fontSize = (i == menuSelection) ? 32 : 26;
        DrawText(options[i], SCREEN_WIDTH / 2 - MeasureText(options[i], fontSize) / 2,
                 300 + i * 54, fontSize, c);
    }

    const char *hint = "Use UP / DOWN and ENTER";
    DrawText(hint, SCREEN_WIDTH / 2 - MeasureText(hint, 18) / 2, 610, 18, YELLOW);
}

void UpdateNameInput(void)
{
    int key = GetCharPressed();
    while (key > 0)
    {
        if (isprint(key) && nameLetterCount < MAX_NAME_LEN)
        {
            playerName[nameLetterCount] = (char)key;
            nameLetterCount++;
            playerName[nameLetterCount] = '\0';
        }
        key = GetCharPressed();
    }

    if (IsKeyPressed(KEY_BACKSPACE) && nameLetterCount > 0)
    {
        nameLetterCount--;
        playerName[nameLetterCount] = '\0';
    }

    if (IsKeyPressed(KEY_ENTER) && nameLetterCount > 0)
    {
        PlayConfirmSfx();
        InitGame();
        currentState = STATE_GAMEPLAY;
    }
}

void DrawNameInput(void)
{
    DrawMenuStyleBackground();

    const char *prompt = "ENTER YOUR NAME:";
    DrawText(prompt, SCREEN_WIDTH / 2 - MeasureText(prompt, 32) / 2, 260, 32, WHITE);

    Rectangle box = {SCREEN_WIDTH / 2.0 - 180, 330, 360, 50};
    DrawRectangleRec(box, WHITE);
    DrawRectangleLinesEx(box, 2, DARKBLUE);
    DrawText(playerName, (int)box.x + 12, (int)box.y + 12, 28, MAROON);

    if (((int)(GetTime() * 2)) % 2 == 0)
    {
        int textW = MeasureText(playerName, 28);
        DrawText("_", (int)box.x + 12 + textW, (int)box.y + 12, 28, MAROON);
    }

    const char *hint = "Press ENTER to continue";
    DrawText(hint, SCREEN_WIDTH / 2 - MeasureText(hint, 18) / 2, 410, 18, WHITE);
}

void UpdateGameplay(void)
{
if (score > highScores[0].score)
{
    highScores[0].score = score;
}
    if (IsKeyPressed(KEY_P) || IsKeyPressed(KEY_ESCAPE))
        isPaused = !isPaused;

    if (isPaused)
    {
        if (IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_DOWN))
            pauseOption = 1 - pauseOption;

        if (IsKeyPressed(KEY_ENTER))
        {
            PlayConfirmSfx();
            if (pauseOption == 0)
                isPaused = false;
            else
            {
                currentState = STATE_MENU;
                isPaused = false;
            }
        }
        return;
    }

    if (IsKeyPressed(KEY_N))
    {
        AwardScore(currentLevel * 100);
        currentState = STATE_LEVEL_CLEAR;
        PlaySound(sfxLevelClear);
        return;
    }
    if (IsKeyPressed(KEY_C))
        paddle.hasLaser = true;

    if (IsKeyDown(KEY_LEFT) || IsKeyDown(KEY_A))
        paddle.rect.x -= paddle.speed;
    if (IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_D))
        paddle.rect.x += paddle.speed;

    if (paddle.rect.x < 0)
        paddle.rect.x = 0;
    if (paddle.rect.x + paddle.rect.width > SCREEN_WIDTH)
        paddle.rect.x = SCREEN_WIDTH - paddle.rect.width;

    if (!ball.active)
    {
        ball.pos.x = paddle.rect.x + paddle.rect.width / 2.0;
        ball.pos.y = paddle.rect.y - ball.radius - 2;

        if (IsKeyPressed(KEY_SPACE))
        {
            SetBallVelocityFromAngle(90.0, GetLevelBallSpeed(currentLevel));
            ball.active = true;
        }
    }
    else
    {
        ball.pos.x += ball.speed.x;
        ball.pos.y += ball.speed.y;

        if (ball.pos.x - ball.radius < 0 || ball.pos.x + ball.radius > SCREEN_WIDTH)
        {
            ReflectBallX();
            PlaySound(sfxWallHit);
        }
        if (ball.pos.y - ball.radius < 0)
        {
            ReflectBallY();
            PlaySound(sfxWallHit);
        }
        if (ball.pos.y + ball.radius > SCREEN_HEIGHT)
        {
            lives--;
            PlaySound(sfxLose);
            ball.active = false;
            ball.speed = (Vector2){0.0, 0.0};
            paddle.rect.width = 120;
            paddle.hasLaser = false;

            if (lives <= 0)
            {
                AddHighScore(playerName, score);
                endSelection = 0;
                currentState = STATE_GAME_OVER;
                PlaySound(sfxGameOver);
                return;
            }
        }

        if (paddle.hasLaser && IsKeyPressed(KEY_SPACE))
            ShootLaser();
    }

    CheckCollisions();
    UpdatePowerUps();
    if (currentState != STATE_GAMEPLAY)
        return;

    UpdateLasers();
    UpdateBomb();

    bool anyBricksLeft = false;
    for (int i = 0; i < ROWS && !anyBricksLeft; i++)
        for (int j = 0; j < COLS; j++)
            if (bricks[i][j].active && !bricks[i][j].unbreakable)
            {
                anyBricksLeft = true;
                break;
            }

    if (!anyBricksLeft)
    {
        AwardScore(100);
        currentState = STATE_LEVEL_CLEAR;
        PlaySound(sfxLevelClear);
    }

}

void CheckCollisions(void)
{
    if (!ball.active)
        return;

    if (ball.speed.y > 0)
    {
        Rectangle sweep = paddle.rect;
        sweep.y -= ball.speed.y;
        sweep.height += ball.speed.y;

        if (CheckCollisionCircleRec(ball.pos, ball.radius, paddle.rect) ||
            CheckCollisionCircleRec(ball.pos, ball.radius, sweep))
        {
            ball.pos.y = paddle.rect.y - ball.radius - 1.0;
            float angleDeg = CalculateBounceAngle(ball.pos, paddle.rect);
            SetBallVelocityFromAngle(angleDeg, GetLevelBallSpeed(currentLevel));
            PlaySound(sfxPaddleHit);
        }
    }

    for (int i = 0; i < ROWS; i++)
    {
        for (int j = 0; j < COLS; j++)
        {
            if (!bricks[i][j].active)
                continue;
            if (!CheckCollisionCircleRec(ball.pos, ball.radius, bricks[i][j].rect))
                continue;

            if (bricks[i][j].unbreakable)
            {
                BounceOffUnbreakable(bricks[i][j].rect);
                return;
            }

            bool destroyed = DamageBrick(i, j);
            ResolveBallBrickCollision(bricks[i][j].rect);

            if (destroyed)
            {
                AwardScore(currentLevel * 10);
                PlaySound(sfxBrickBreak);
                Vector2 pos = {bricks[i][j].rect.x + bricks[i][j].rect.width / 2,
                               bricks[i][j].rect.y};
                SpawnPowerUp(pos);
            }
            return;
        }
    }
}

void DrawBrick(const Brick *b)
{
    DrawRectangleRec(b->rect, b->color);
    DrawRectangleLinesEx(b->rect, 2, COLOR_BRICK_BORDER);

    if (!b->unbreakable && b->maxHits > 1)
    {
        char label[4];
        sprintf(label, "%d", b->hitPoints);
        int tw = MeasureText(label, 14);
        DrawText(label,
                 (int)(b->rect.x + b->rect.width / 2 - tw / 2),
                 (int)(b->rect.y + b->rect.height / 2 - 7),
                 14, WHITE);
    }
}

void DrawHUD(void)
{
    int hudY = 35;
    int fontSize = 22;

    DrawText(TextFormat("SCORE: %d", score), 40, hudY, fontSize, ORANGE);
    DrawText(TextFormat("LEVEL: %d", currentLevel), 200, hudY, fontSize, RED);

    int topScore = (highScoreCount > 0) ? highScores[0].score : 0;
    DrawText(TextFormat("HIGH SCORE: %d", topScore), 510, hudY, fontSize, ORANGE);
   DrawText(TextFormat("AUDIO: %s (M)", isAudioMuted ? "OFF" : "ON"), 850, hudY, fontSize, isAudioMuted ? RED : GREEN);

    DrawText(TextFormat("LIVES: %d", lives), 1140, hudY, fontSize, RED);
}



void DrawGameplay(void)
{
    DrawGameplayStyleBackground();

    for (int i = 0; i < ROWS; i++)
        for (int j = 0; j < COLS; j++)
            if (bricks[i][j].active)
                DrawBrick(&bricks[i][j]);

   
    DrawRectangleRec(paddle.rect, BLUE);
    if (paddle.hasLaser)
    {
        DrawRectangle((int)paddle.rect.x + 3, (int)paddle.rect.y - 6, 6, 6, RED);
        DrawRectangle((int)(paddle.rect.x + paddle.rect.width - 9),
                      (int)paddle.rect.y - 6, 6, 6, RED);
    }

    // Ball ebong Launch Text-er jonno Bouncing Animation Offset calculate kora
    float bounceOffset = 0.0f;
    if (!ball.active && !isPaused)
    {
        bounceOffset = sinf((float)GetTime() * 6.0f) * 6.0f;
    }

    // 2. Ball launch na hwa porjonto ball-ti paddle-er shathe opor-niche lafabe
    float currentBallX = ball.pos.x;
    float currentBallY = ball.pos.y;
    if (!ball.active && !isPaused)
    {
        currentBallX = paddle.rect.x + paddle.rect.width / 2.0f;
        currentBallY = paddle.rect.y - ball.radius + bounceOffset;
    }

    DrawTexture(ballTexture,
                (int)(currentBallX - ballTexture.width / 2),
                (int)(currentBallY - ballTexture.height / 2),
                WHITE);

    DrawPowerUps();
    DrawLasers();
    DrawBomb();
    DrawHUD();

    
    if (!ball.active && !isPaused)
    {
        const char *msg = "PRESS SPACE TO LAUNCH";
        DrawText(msg, SCREEN_WIDTH / 2 - MeasureText(msg, 22) / 2,
                 SCREEN_HEIGHT / 2 + 100 + (int)bounceOffset, 22, WHITE);
    }

    if (isPaused)
    {
        DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, Fade(BLACK, 0.65));
        DrawText("GAME PAUSED",
                 SCREEN_WIDTH / 2 - MeasureText("GAME PAUSED", 42) / 2, 220, 42, GOLD);

        Color continueColor = (pauseOption == 0) ? YELLOW : WHITE;
        const char *continueTxt = (pauseOption == 0) ? "> CONTINUE <" : "  CONTINUE  ";
        DrawText(continueTxt,
                 SCREEN_WIDTH / 2 - MeasureText(continueTxt, 24) / 2, 330, 24, continueColor);

        Color exitColor = (pauseOption == 1) ? YELLOW : WHITE;
        const char *exitTxt = (pauseOption == 1) ? "> EXIT TO MENU <" : "  EXIT TO MENU  ";
        DrawText(exitTxt,
                 SCREEN_WIDTH / 2 - MeasureText(exitTxt, 24) / 2, 380, 24, exitColor);
    }
}

void UpdateLevelClear(void)
{
    if (!IsKeyPressed(KEY_ENTER))
        return;
    PlayConfirmSfx();

    if (currentLevel >= MAX_LEVELS)
    {
        AwardScore(200);
        AddHighScore(playerName, score);
        PlaySound(sfxWinFanfare);
        endSelection = 0;
        currentState = STATE_WIN;
    }
    else
    {
        currentLevel++;
        LoadLevel(currentLevel);
        ClearPowerUpsAndLasers();
        ResetPaddleAndBall();
        currentState = STATE_GAMEPLAY;
    }
}

void DrawLevelClear(void)
{
    DrawGameplayStyleBackground();
    DrawRectangleRec(COLOR_PANEL_RECT, COLOR_PANEL);

    const char *msg = "LEVEL CLEAR!";
    DrawText(msg, SCREEN_WIDTH / 2 - MeasureText(msg, 46) / 2,
             SCREEN_HEIGHT / 2 - 80, 46, GOLD);

    const char *scoreLine = TextFormat("Score: %i", score);
    DrawText(scoreLine, SCREEN_WIDTH / 2 - MeasureText(scoreLine, 26) / 2,
             SCREEN_HEIGHT / 2 - 10, 26, WHITE);

    const char *hint = (currentLevel >= MAX_LEVELS)
                           ? "PRESS ENTER TO SEE YOUR RESULT"
                           : "PRESS ENTER FOR THE NEXT LEVEL";
    DrawText(hint, SCREEN_WIDTH / 2 - MeasureText(hint, 20) / 2,
             SCREEN_HEIGHT / 2 + 50, 20, WHITE);
}

bool UpdateEndScreenNav(void)
{
    if (IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_UP))
        endSelection = 1 - endSelection;
    if (IsKeyPressed(KEY_ENTER))
    {
        PlayConfirmSfx();
        return true;
    }
    return false;
}

void UpdateGameOver(void)
{
    if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_R))
    {
        PlayConfirmSfx();
        currentState = STATE_MENU; 
    }
}

void UpdateWin(void)
{
    if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_R))
    {
        PlayConfirmSfx();
        currentState = STATE_MENU;
    }
}

void DrawEndScreen(const char *title, Color titleColor)
{
    DrawMenuStyleBackground();

    DrawText(title, SCREEN_WIDTH / 2 - MeasureText(title, 50) / 2, 150, 50, titleColor);

    const char *nameLine = TextFormat("Player: %s", playerName);
    DrawText(nameLine, SCREEN_WIDTH / 2 - MeasureText(nameLine, 26) / 2, 240, 26, WHITE);

    const char *scoreLine = TextFormat("Final Score: %i", score);
    DrawText(scoreLine, SCREEN_WIDTH / 2 - MeasureText(scoreLine, 26) / 2, 280, 26, WHITE);

    const char *opts[2] = {"Retry", "Exit"};
    for (int i = 0; i < 2; i++)
    {
        Color c = (i == endSelection) ? YELLOW : WHITE;
        int fontSize = (i == endSelection) ? 34 : 28;
        DrawText(opts[i], SCREEN_WIDTH / 2 - MeasureText(opts[i], fontSize) / 2,
                 400 + i * 60, fontSize, c);
    }

    const char *hint = "Use UP / DOWN and ENTER";
    DrawText(hint, SCREEN_WIDTH / 2 - MeasureText(hint, 18) / 2, 560, 18, YELLOW);
}

void DrawGameOver(void) { DrawEndScreen("GAME OVER", RED); }
void DrawWin(void) { DrawEndScreen("YOU WIN!", GREEN); }

void UpdateHighScores(void)
{
    if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_ESCAPE))
    {
        PlayConfirmSfx();
        currentState = STATE_MENU;
        menuSelection = 0;
    }
}

void DrawHighScores(void)
{
    DrawMenuStyleBackground();

    const char *title = "HIGH SCORES";
    DrawText(title, SCREEN_WIDTH / 2 - MeasureText(title, 46) / 2, 120, 46, GOLD);

    if (highScoreCount == 0)
    {
        const char *empty = "No scores yet -- be the first!";
        DrawText(empty, SCREEN_WIDTH / 2 - MeasureText(empty, 22) / 2, 300, 22, WHITE);
    }
    else
    {
        for (int i = 0; i < highScoreCount; i++)
        {
            const char *line = TextFormat("%d. %-15s %d", i + 1,
                                          highScores[i].name, highScores[i].score);
            DrawText(line, SCREEN_WIDTH / 2 - MeasureText(line, 26) / 2,
                     240 + i * 46, 26, WHITE);
        }
    }

    const char *hint = "PRESS ENTER TO RETURN TO MENU";
    DrawText(hint, SCREEN_WIDTH / 2 - MeasureText(hint, 18) / 2, 600, 18, YELLOW);
}

void UpdateHowToPlay(void)

{
    if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_ESCAPE))
    {
        PlayConfirmSfx();
        currentState = STATE_MENU;
        menuSelection = 0;
    }
}

void DrawHowToPlay(void)
{
    DrawMenuStyleBackground();

    const char *title = "HOW TO PLAY";
    DrawText(title, SCREEN_WIDTH / 2 - MeasureText(title, 44) / 2, 78, 44, GOLD);

    int leftX = 230;
    int rightX = 700;
    int y = 150;
    int lineGap = 24;

    DrawText("CONTROLS", leftX, y, 24, YELLOW);
    y += lineGap + 4;
    DrawText("LEFT / A         Move paddle left", leftX, y, 17, WHITE);
    y += lineGap;
    DrawText("RIGHT / D        Move paddle right", leftX, y, 17, WHITE);
    y += lineGap;
    DrawText("SPACE            Launch ball / Fire laser", leftX, y, 17, WHITE);
    y += lineGap;
    DrawText("P / ESC          Pause game", leftX, y, 17, WHITE);
    y += lineGap;
    DrawText("M                Toggle BGM (Mute/Unmute)", leftX, y, 17, WHITE); // New controls line
    y += lineGap;
    DrawText("ENTER            Confirm / Continue", leftX, y, 17, WHITE);
    y += lineGap + 10;

    DrawText("GOAL", leftX, y, 24, YELLOW);
    y += lineGap + 4;
    DrawText("Break every breakable brick to clear", leftX, y, 17, WHITE);
    y += lineGap;
    DrawText("each level. Gold bricks are", leftX, y, 17, WHITE);
    y += lineGap;
    DrawText("unbreakable -- the ball just bounces", leftX, y, 17, WHITE);
    y += lineGap;
    DrawText("off them. Don't let the ball fall", leftX, y, 17, WHITE);
    y += lineGap;
    DrawText("past your paddle, or you lose a life!", leftX, y, 17, WHITE);

    int ry = 150;
    DrawText("POWER-UPS", rightX, ry, 24, YELLOW);
    ry += lineGap + 4;

    struct
    {
        Color c;
        const char *label;
        const char *desc;
    } powerInfo[7] = {
        {BLUE, "W", "Expand Paddle - widens paddle"},
        {GREEN, "+", "Extra Life - gain one life"},
        {ORANGE, "S", "Slow Ball - slows the ball"},
        {PURPLE, "L", "Laser - fire lasers"},
        {RED, "X", "Anti-Life - costs one life"},
        {GOLD, "$", "Bonus Score - instant score"},
        {MAROON, "B", "Bomb - destroys bricks"},
    };

    for (int i = 0; i < 7; i++)
    {
        DrawCircleV((Vector2){(float)(rightX + 10), (float)(ry + 9)}, 12, powerInfo[i].c);
        DrawText(powerInfo[i].label, rightX + 6, ry + 2, 14, WHITE);
        DrawText(powerInfo[i].desc, rightX + 32, ry + 1, 16, WHITE);
        ry += lineGap;
    }

    const char *hint = "PRESS ENTER TO RETURN TO MENU";
    DrawText(hint, SCREEN_WIDTH / 2 - MeasureText(hint, 18) / 2, 600, 18, YELLOW);
}

void UpdateCredits(void)
{
    if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_ESCAPE))
    {
        PlayConfirmSfx();
        currentState = STATE_MENU;
        menuSelection = 0;
    }
}

void DrawCredits(void)
{
    DrawMenuStyleBackground();

    const char *title = "CREDITS";
    DrawText(title, SCREEN_WIDTH / 2 - MeasureText(title, 46) / 2, 100, 46, GOLD);

    /* ---- Credits ---- */
    const char *creditLines[] = {
        "SUPERVISOR:",
        "Dr. Mohammad Mahfuzul Islam",
        "",
        "Game design & programming:",
        " Al Mukaddim Saki(2505068) & Naeema Noorjahan(2505069)",
        "",
        "GAME ENGINE/FRAMEWORK:",
        "raylib",
        "",

        "SOUND EFFECTS & MUSIC:",
        "  Free .wav sound effects from Mixkit.co",
        "",
        "BACKGROUND IMAGES:",
        " https://unsplash.com/ && https://eee.buet.ac.bd/outreach/branding",
        ""

    };
    int lineCount = sizeof(creditLines) / sizeof(creditLines[0]);

    int y = 175;
    for (int i = 0; i < lineCount; i++)
    {
        Color c = WHITE;
        int fontSize = 20;
        if (creditLines[i][0] != '\0' && creditLines[i][strlen(creditLines[i]) - 1] == ':')
        {
            c = YELLOW;
            fontSize = 22;
        }
        DrawText(creditLines[i], SCREEN_WIDTH / 2 - MeasureText(creditLines[i], fontSize) / 2,
                 y, fontSize, c);
        y += 28;
    }

    const char *hint = "PRESS ENTER TO RETURN TO MENU";
    DrawText(hint, SCREEN_WIDTH / 2 - MeasureText(hint, 18) / 2, 630, 18, GOLD);
}