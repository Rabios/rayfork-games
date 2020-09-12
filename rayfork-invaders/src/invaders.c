/*******************************************************************************************
*
*   raylib - sample game: space invaders
*
*   Sample game developed by Ian Eito, Albert Martos and Ramon Santamaria
*
*   This game has been created using raylib v1.3 (www.raylib.com)
*   raylib is licensed under an unmodified zlib/libpng license (View raylib.h for details)
*
*   Copyright (c) 2015 Ramon Santamaria (@raysan5)
*
********************************************************************************************/
// Ported to rayfork by Rabia Alhaffar in 12/September/2020
#include "platform.h"

//----------------------------------------------------------------------------------
// Some Defines
//----------------------------------------------------------------------------------
#define NUM_SHOOTS 50
#define NUM_MAX_ENEMIES 50
#define FIRST_WAVE 10
#define SECOND_WAVE 20
#define THIRD_WAVE 50

//----------------------------------------------------------------------------------
// Types and Structures Definition
//----------------------------------------------------------------------------------
typedef enum { FIRST = 0, SECOND, THIRD } EnemyWave;

typedef struct Player {
    rf_rec rec;
    rf_vec2 speed;
    rf_color color;
} Player;

typedef struct Enemy {
    rf_rec rec;
    rf_vec2 speed;
    bool active;
    rf_color color;
} Enemy;

typedef struct Shoot {
    rf_rec rec;
    rf_vec2 speed;
    bool active;
    rf_color color;
} Shoot;

//------------------------------------------------------------------------------------
// Global Variables Declaration
//------------------------------------------------------------------------------------
static bool gameOver = false;
static bool pause = false;
static int score = 0;
static bool victory = false;

static Player player = { 0 };
static Enemy enemy[NUM_MAX_ENEMIES] = { 0 };
static Shoot shoot[NUM_SHOOTS] = { 0 };
static EnemyWave wave = { 0 };

static int shootRate = 0;
static float alpha = 0.0f;

static int activeEnemies = 0;
static int enemiesKill = 0;
static bool smooth = false;

//------------------------------------------------------------------------------------
// Module Functions Declaration (local)
//------------------------------------------------------------------------------------
static void InitGame(void);                                      // Initialize game
static void UpdateGame(const platform_input_state* input);       // Update game (one frame)
static void DrawGame(void);                                      // Draw game (one frame)
static void UnloadGame(void);                                    // Unload game
static void UpdateDrawFrame(const platform_input_state* input);  // Update and Draw (one frame)

// Formatting of text with variables to 'embed'
// WARNING: String returned will expire after this function is called MAX_TEXTFORMAT_BUFFERS times
#define MAX_TEXT_BUFFER_LENGTH              1024        // Size of internal static buffers used on some functions:
const char* TextFormat(const char* text, ...)
{
#ifndef MAX_TEXTFORMAT_BUFFERS
#define MAX_TEXTFORMAT_BUFFERS 4        // Maximum number of static buffers for text formatting
#endif

    // We create an array of buffers so strings don't expire until MAX_TEXTFORMAT_BUFFERS invocations
    static char buffers[MAX_TEXTFORMAT_BUFFERS][MAX_TEXT_BUFFER_LENGTH] = { 0 };
    static int  index = 0;

    char* currentBuffer = buffers[index];
    memset(currentBuffer, 0, MAX_TEXT_BUFFER_LENGTH);   // Clear buffer before using

    va_list args;
    va_start(args, text);
    vsprintf(currentBuffer, text, args);
    va_end(args);

    index += 1;     // Move to next buffer for next function call
    if (index >= MAX_TEXTFORMAT_BUFFERS) index = 0;

    return currentBuffer;
}

// Returns a random value between min and max (both included)
int GetRandomValue(int min, int max)
{
    if (min > max)
    {
        int tmp = max;
        max = min;
        min = tmp;
    }

    return (rand() % (abs(max - min) + 1) + min);
}

//------------------------------------------------------------------------------------
// Program main entry point
//------------------------------------------------------------------------------------
platform_window_details window = {
    .width  = 800,
    .height = 450,
    .title  = "sample game: space invaders"
};

rf_context ctx;
rf_render_batch batch;
const platform_input_state* input;

extern void game_init(rf_gfx_backend_data* gfx_data)
{
    // Initialization
    //---------------------------------------------------------

    // Initialize rayfork
    rf_init_context(&ctx);
    rf_init_gfx(window.width, window.height, gfx_data);

    // Initialize the rendering batch
    batch = rf_create_default_render_batch(RF_DEFAULT_ALLOCATOR);
    rf_set_active_render_batch(&batch);

    InitGame();
}

extern void game_update(const platform_input_state* input)
{
    UpdateDrawFrame(input);
}

//------------------------------------------------------------------------------------
// Module Functions Definitions (local)
//------------------------------------------------------------------------------------

// Initialize game variables
void InitGame(void)
{
    // Initialize game variables
    shootRate = 0;
    pause = false;
    gameOver = false;
    victory = false;
    smooth = false;
    wave = FIRST;
    activeEnemies = FIRST_WAVE;
    enemiesKill = 0;
    score = 0;
    alpha = 0;

    // Initialize player
    player.rec.x = 20;
    player.rec.y = 50;
    player.rec.width = 20;
    player.rec.height = 20;
    player.speed.x = 5;
    player.speed.y = 5;
    player.color = RF_BLACK;

    // Initialize enemies
    for (int i = 0; i < NUM_MAX_ENEMIES; i++)
    {
        enemy[i].rec.width = 10;
        enemy[i].rec.height = 10;
        enemy[i].rec.x = GetRandomValue(window.width, window.width + 1000);
        enemy[i].rec.y = GetRandomValue(0, window.height - enemy[i].rec.height);
        enemy[i].speed.x = 5;
        enemy[i].speed.y = 5;
        enemy[i].active = true;
        enemy[i].color = RF_GRAY;
    }

    // Initialize shoots
    for (int i = 0; i < NUM_SHOOTS; i++)
    {
        shoot[i].rec.x = player.rec.x;
        shoot[i].rec.y = player.rec.y + player.rec.height / 4;
        shoot[i].rec.width = 10;
        shoot[i].rec.height = 5;
        shoot[i].speed.x = 7;
        shoot[i].speed.y = 0;
        shoot[i].active = false;
        shoot[i].color = RF_MAROON;
    }
}

// Update game (one frame)
void UpdateGame(const platform_input_state* input)
{
    if (!gameOver)
    {
        if (input->keys[KEYCODE_P] == KEY_PRESSED_DOWN) pause = !pause;

        if (!pause)
        {
            switch (wave)
            {
            case FIRST:
            {
                if (!smooth)
                {
                    alpha += 0.02f;

                    if (alpha >= 1.0f) smooth = true;
                }

                if (smooth) alpha -= 0.02f;

                if (enemiesKill == activeEnemies)
                {
                    enemiesKill = 0;

                    for (int i = 0; i < activeEnemies; i++)
                    {
                        if (!enemy[i].active) enemy[i].active = true;
                    }

                    activeEnemies = SECOND_WAVE;
                    wave = SECOND;
                    smooth = false;
                    alpha = 0.0f;
                }
            } break;
            case SECOND:
            {
                if (!smooth)
                {
                    alpha += 0.02f;

                    if (alpha >= 1.0f) smooth = true;
                }

                if (smooth) alpha -= 0.02f;

                if (enemiesKill == activeEnemies)
                {
                    enemiesKill = 0;

                    for (int i = 0; i < activeEnemies; i++)
                    {
                        if (!enemy[i].active) enemy[i].active = true;
                    }

                    activeEnemies = THIRD_WAVE;
                    wave = THIRD;
                    smooth = false;
                    alpha = 0.0f;
                }
            } break;
            case THIRD:
            {
                if (!smooth)
                {
                    alpha += 0.02f;

                    if (alpha >= 1.0f) smooth = true;
                }

                if (smooth) alpha -= 0.02f;

                if (enemiesKill == activeEnemies) victory = true;

            } break;
            default: break;
            }

            // Player movement
            if (input->keys[KEYCODE_RIGHT] == KEY_HOLD_DOWN) player.rec.x += player.speed.x;
            if (input->keys[KEYCODE_LEFT] == KEY_HOLD_DOWN) player.rec.x -= player.speed.x;
            if (input->keys[KEYCODE_UP] == KEY_HOLD_DOWN) player.rec.y -= player.speed.y;
            if (input->keys[KEYCODE_DOWN] == KEY_HOLD_DOWN) player.rec.y += player.speed.y;

            // Player collision with enemy
            for (int i = 0; i < activeEnemies; i++)
            {
                if (rf_check_collision_recs(player.rec, enemy[i].rec)) gameOver = true;
            }

            // Enemy behaviour
            for (int i = 0; i < activeEnemies; i++)
            {
                if (enemy[i].active)
                {
                    enemy[i].rec.x -= enemy[i].speed.x;

                    if (enemy[i].rec.x < 0)
                    {
                        enemy[i].rec.x = GetRandomValue(window.width, window.width + 1000);
                        enemy[i].rec.y = GetRandomValue(0, window.height - enemy[i].rec.height);
                    }
                }
            }

            // Wall behaviour
            if (player.rec.x <= 0) player.rec.x = 0;
            if (player.rec.x + player.rec.width >= window.width) player.rec.x = window.width - player.rec.width;
            if (player.rec.y <= 0) player.rec.y = 0;
            if (player.rec.y + player.rec.height >= window.height) player.rec.y = window.height - player.rec.height;

            // Shoot initialization
            if (input->keys[KEYCODE_SPACE] == KEY_HOLD_DOWN)
            {
                shootRate += 5;

                for (int i = 0; i < NUM_SHOOTS; i++)
                {
                    if (!shoot[i].active && shootRate % 20 == 0)
                    {
                        shoot[i].rec.x = player.rec.x;
                        shoot[i].rec.y = player.rec.y + player.rec.height / 4;
                        shoot[i].active = true;
                        break;
                    }
                }
            }

            // Shoot logic
            for (int i = 0; i < NUM_SHOOTS; i++)
            {
                if (shoot[i].active)
                {
                    // Movement
                    shoot[i].rec.x += shoot[i].speed.x;

                    // Collision with enemy
                    for (int j = 0; j < activeEnemies; j++)
                    {
                        if (enemy[j].active)
                        {
                            if (rf_check_collision_recs(shoot[i].rec, enemy[j].rec))
                            {
                                shoot[i].active = false;
                                enemy[j].rec.x = GetRandomValue(window.width, window.width + 1000);
                                enemy[j].rec.y = GetRandomValue(0, window.height - enemy[j].rec.height);
                                shootRate = 0;
                                enemiesKill++;
                                score += 100;
                            }

                            if (shoot[i].rec.x + shoot[i].rec.width >= window.width)
                            {
                                shoot[i].active = false;
                                shootRate = 0;
                            }
                        }
                    }
                }
            }
        }
    }
    else
    {
        if (input->keys[KEYCODE_ENTER] == KEY_PRESSED_DOWN)
        {
            InitGame();
            gameOver = false;
        }
    }
}

// Draw game (one frame)
void DrawGame(void)
{
    rf_begin();
    {
        rf_clear(RF_RAYWHITE);

        if (!gameOver)
        {
            rf_draw_rectangle_rec(player.rec, player.color);

            if (wave == FIRST) rf_draw_text("FIRST WAVE", window.width / 2 - rf_measure_text(rf_get_default_font(), "FIRST WAVE", 40, 0.0f).width / 2, window.height / 2 - 40, 40, rf_fade(RF_BLACK, alpha));
            else if (wave == SECOND) rf_draw_text("SECOND WAVE", window.width / 2 - rf_measure_text(rf_get_default_font(), "SECOND WAVE", 40, 0.0f).width / 2, window.height / 2 - 40, 40, rf_fade(RF_BLACK, alpha));
            else if (wave == THIRD) rf_draw_text("THIRD WAVE", window.width / 2 - rf_measure_text(rf_get_default_font(), "THIRD WAVE", 40, 0.0f).width / 2, window.height / 2 - 40, 40, rf_fade(RF_BLACK, alpha));

            for (int i = 0; i < activeEnemies; i++)
            {
                if (enemy[i].active) rf_draw_rectangle_rec(enemy[i].rec, enemy[i].color);
            }

            for (int i = 0; i < NUM_SHOOTS; i++)
            {
                if (shoot[i].active) rf_draw_rectangle_rec(shoot[i].rec, shoot[i].color);
            }

            rf_draw_text(TextFormat("%04i", score), 20, 20, 40, RF_GRAY);

            if (victory) rf_draw_text("YOU WIN", window.width / 2 - rf_measure_text(rf_get_default_font(), "YOU WIN", 40, 0.0f).width / 2, window.height / 2 - 40, 40, RF_BLACK);

            if (pause) rf_draw_text("GAME PAUSED", window.width / 2 - rf_measure_text(rf_get_default_font(), "GAME PAUSED", 40, 0.0f).width / 2, window.height / 2 - 40, 40, RF_GRAY);
        }
        else rf_draw_text("PRESS [ENTER] TO PLAY AGAIN", window.width / 2 - rf_measure_text(rf_get_default_font(), "PRESS [ENTER] TO PLAY AGAIN", 20, 0.0f).width / 2, window.height / 2 - 50, 20, RF_GRAY);
    }
    rf_end();
}

// Unload game variables
void UnloadGame(void)
{
    // TODO: Unload all dynamic loaded data (textures, sounds, models...)
}

// Update and Draw (one frame)
void UpdateDrawFrame(const platform_input_state* input)
{
    UpdateGame(input);
    DrawGame();
}