/*******************************************************************************************
*
*   raylib - sample game: gold fever
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
// Types and Structures Definition
//----------------------------------------------------------------------------------
typedef struct Player {
    rf_vec2 position;
    rf_vec2 speed;
    int radius;
} Player;

typedef struct Enemy {
    rf_vec2 position;
    rf_vec2 speed;
    int radius;
    int radiusBounds;
    bool moveRight;
} Enemy;

typedef struct Points {
    rf_vec2 position;
    int radius;
    int value;
    bool active;
} Points;

typedef struct Home {
    rf_rec rec;
    bool active;
    bool save;
    rf_color color;
} Home;

//------------------------------------------------------------------------------------
// Global Variables Declaration
//------------------------------------------------------------------------------------
static bool gameOver = false;
static bool pause = false;
static int score = 0;
static int hiScore = 0;

static Player player = { 0 };
static Enemy enemy = { 0 };
static Points points = { 0 };
static Home home = { 0 };
static bool follow = false;

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
    .title  = "sample game: gold fever"
};

rf_context ctx;
rf_render_batch batch;
const platform_input_state* input;

extern void game_init(rf_gfx_backend_data* gfx_data)
{
    // Initialize rayfork
    rf_init_context(&ctx);
    rf_init_gfx(window.width, window.height, gfx_data);

    // Initialize the rendering batch
    batch = rf_create_default_render_batch(RF_DEFAULT_ALLOCATOR);
    rf_set_active_render_batch(&batch);

    // Initialization (Note windowTitle is unused on Android)
    //---------------------------------------------------------
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
    pause = false;
    score = 0;

    player.position = (rf_vec2){ 50, 50 };
    player.radius = 20;
    player.speed = (rf_vec2){ 5, 5 };

    enemy.position = (rf_vec2){ window.width - 50, window.height / 2 };
    enemy.radius = 20;
    enemy.radiusBounds = 150;
    enemy.speed = (rf_vec2){ 3, 3 };
    enemy.moveRight = true;
    follow = false;

    points.radius = 10;
    points.position = (rf_vec2){ GetRandomValue(points.radius, window.width - points.radius), GetRandomValue(points.radius, window.height - points.radius) };
    points.value = 100;
    points.active = true;

    home.rec.width = 50;
    home.rec.height = 50;
    home.rec.x = GetRandomValue(0, window.width - home.rec.width);
    home.rec.y = GetRandomValue(0, window.height - home.rec.height);
    home.active = false;
    home.save = false;
}

// Update game (one frame)
void UpdateGame(const platform_input_state* input)
{
    if (!gameOver)
    {
        if (input->keys[KEYCODE_P] == KEY_PRESSED_DOWN) pause = !pause;

        if (!pause)
        {
            // Control player
            if (input->keys[KEYCODE_RIGHT] == KEY_HOLD_DOWN) player.position.x += player.speed.x;
            if (input->keys[KEYCODE_LEFT] == KEY_HOLD_DOWN) player.position.x -= player.speed.x;
            if (input->keys[KEYCODE_UP] == KEY_HOLD_DOWN) player.position.y -= player.speed.y;
            if (input->keys[KEYCODE_DOWN] == KEY_HOLD_DOWN) player.position.y += player.speed.y;

            // Wall behaviour player
            if (player.position.x - player.radius <= 0) player.position.x = player.radius;
            if (player.position.x + player.radius >= window.width) player.position.x = window.width - player.radius;
            if (player.position.y - player.radius <= 0) player.position.y = player.radius;
            if (player.position.y + player.radius >= window.height) player.position.y = window.height - player.radius;

            // IA Enemy
            if ((follow || rf_check_collision_circles(player.position, player.radius, enemy.position, enemy.radiusBounds)) && !home.save)
            {
                if (player.position.x > enemy.position.x) enemy.position.x += enemy.speed.x;
                if (player.position.x < enemy.position.x) enemy.position.x -= enemy.speed.x;

                if (player.position.y > enemy.position.y) enemy.position.y += enemy.speed.y;
                if (player.position.y < enemy.position.y) enemy.position.y -= enemy.speed.y;
            }
            else
            {
                if (enemy.moveRight) enemy.position.x += enemy.speed.x;
                else enemy.position.x -= enemy.speed.x;
            }

            // Wall behaviour enemy
            if (enemy.position.x - enemy.radius <= 0) enemy.moveRight = true;
            if (enemy.position.x + enemy.radius >= window.width) enemy.moveRight = false;

            if (enemy.position.x - enemy.radius <= 0) enemy.position.x = enemy.radius;
            if (enemy.position.x + enemy.radius >= window.width) enemy.position.x = window.width - enemy.radius;
            if (enemy.position.y - enemy.radius <= 0) enemy.position.y = enemy.radius;
            if (enemy.position.y + enemy.radius >= window.height) enemy.position.y = window.height - enemy.radius;

            // Collisions
            if (rf_check_collision_circles(player.position, player.radius, points.position, points.radius) && points.active)
            {
                follow = true;
                points.active = false;
                home.active = true;
            }

            if (rf_check_collision_circles(player.position, player.radius, enemy.position, enemy.radius) && !home.save)
            {
                gameOver = true;

                if (hiScore < score) hiScore = score;
            }

            if (rf_check_collision_circle_rec(player.position, player.radius, home.rec))
            {
                follow = false;

                if (!points.active)
                {
                    score += points.value;
                    points.active = true;
                    enemy.speed.x += 0.5;
                    enemy.speed.y += 0.5;
                    points.position = (rf_vec2){ GetRandomValue(points.radius, window.width - points.radius), GetRandomValue(points.radius, window.height - points.radius) };
                }

                home.save = true;
            }
            else home.save = false;
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
            if (follow)
            {
                rf_draw_rectangle(0, 0, window.width, window.height, RF_RED);
                rf_draw_rectangle(10, 10, window.width - 20, window.height - 20, RF_RAYWHITE);
            }

            rf_draw_rectangle_outline((rf_rec) { home.rec.x, home.rec.y, home.rec.width, home.rec.height }, 1, RF_BLUE);

            rf_draw_circle_lines(enemy.position.x, enemy.position.y, enemy.radiusBounds, RF_RED);
            rf_draw_circle_v(enemy.position, enemy.radius, RF_MAROON);

            rf_draw_circle_v(player.position, player.radius, RF_GRAY);
            if (points.active) rf_draw_circle_v(points.position, points.radius, RF_GOLD);

            rf_draw_text(TextFormat("SCORE: %04i", score), 20, 15, 20, RF_GRAY);
            rf_draw_text(TextFormat("HI-SCORE: %04i", hiScore), 300, 15, 20, RF_GRAY);

            if (pause) rf_draw_text("GAME PAUSED", window.width / 2 - rf_measure_text(rf_get_default_font(), "GAME PAUSED", 40, 0).width / 2, window.height / 2 - 40, 40, RF_GRAY);
        }
        else rf_draw_text("PRESS [ENTER] TO PLAY AGAIN", window.width / 2 - rf_measure_text(rf_get_default_font(), "PRESS [ENTER] TO PLAY AGAIN", 20, 0).width / 2, window.height / 2 - 50, 20, RF_GRAY);
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