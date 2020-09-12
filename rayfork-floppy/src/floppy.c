/*******************************************************************************************
*
*   raylib - sample game: floppy
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
#define MAX_TUBES 100
#define FLOPPY_RADIUS 24
#define TUBES_WIDTH 80

//----------------------------------------------------------------------------------
// Types and Structures Definition
//----------------------------------------------------------------------------------
typedef struct Floppy {
    rf_vec2 position;
    int radius;
    rf_color color;
} Floppy;

typedef struct Tubes {
    rf_rec rec;
    rf_color color;
    bool active;
} Tubes;

//------------------------------------------------------------------------------------
// Global Variables Declaration
//------------------------------------------------------------------------------------
static bool gameOver = false;
static bool pause = false;
static int score = 0;
static int hiScore = 0;

static Floppy floppy = { 0 };
static Tubes tubes[MAX_TUBES * 2] = { 0 };
static rf_vec2 tubesPos[MAX_TUBES] = { 0 };
static int tubesSpeedX = 0;
static bool superfx = false;

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
    .title  = "sample game: floppy"
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

// Initialize game variables
void InitGame(void)
{
    floppy.radius = FLOPPY_RADIUS;
    floppy.position = (rf_vec2){ 80, window.height / 2 - floppy.radius };
    tubesSpeedX = 2;

    for (int i = 0; i < MAX_TUBES; i++)
    {
        tubesPos[i].x = 400 + 280 * i;
        tubesPos[i].y = -GetRandomValue(0, 120);
    }

    for (int i = 0; i < MAX_TUBES * 2; i += 2)
    {
        tubes[i].rec.x = tubesPos[i / 2].x;
        tubes[i].rec.y = tubesPos[i / 2].y;
        tubes[i].rec.width = TUBES_WIDTH;
        tubes[i].rec.height = 255;

        tubes[i + 1].rec.x = tubesPos[i / 2].x;
        tubes[i + 1].rec.y = 600 + tubesPos[i / 2].y - 255;
        tubes[i + 1].rec.width = TUBES_WIDTH;
        tubes[i + 1].rec.height = 255;

        tubes[i / 2].active = true;
    }

    score = 0;

    gameOver = false;
    superfx = false;
    pause = false;
}

// Update game (one frame)
void UpdateGame(const platform_input_state* input)
{
    if (!gameOver)
    {
        if (input->keys[KEYCODE_P] == KEY_PRESSED_DOWN) pause = !pause;

        if (!pause)
        {
            for (int i = 0; i < MAX_TUBES; i++) tubesPos[i].x -= tubesSpeedX;

            for (int i = 0; i < MAX_TUBES * 2; i += 2)
            {
                tubes[i].rec.x = tubesPos[i / 2].x;
                tubes[i + 1].rec.x = tubesPos[i / 2].x;
            }

            if (input->keys[KEYCODE_SPACE] == KEY_HOLD_DOWN && !gameOver) floppy.position.y -= 3;
            else floppy.position.y += 1;

            // Check Collisions
            for (int i = 0; i < MAX_TUBES * 2; i++)
            {
                if (rf_check_collision_circle_rec(floppy.position, floppy.radius, tubes[i].rec))
                {
                    gameOver = true;
                    pause = false;
                }
                else if ((tubesPos[i / 2].x < floppy.position.x) && tubes[i / 2].active && !gameOver)
                {
                    score += 100;
                    tubes[i / 2].active = false;

                    superfx = true;

                    if (score > hiScore) hiScore = score;
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
            rf_draw_circle(floppy.position.x, floppy.position.y, floppy.radius, RF_DARKGRAY);

            // Draw tubes
            for (int i = 0; i < MAX_TUBES; i++)
            {
                rf_draw_rectangle(tubes[i * 2].rec.x, tubes[i * 2].rec.y, tubes[i * 2].rec.width, tubes[i * 2].rec.height, RF_GRAY);
                rf_draw_rectangle(tubes[i * 2 + 1].rec.x, tubes[i * 2 + 1].rec.y, tubes[i * 2 + 1].rec.width, tubes[i * 2 + 1].rec.height, RF_GRAY);
            }

            // Draw flashing fx (one frame only)
            if (superfx)
            {
                rf_draw_rectangle(0, 0, window.width, window.height, RF_WHITE);
                superfx = false;
            }
            rf_draw_text(TextFormat("%04i", score), 20, 20, 40, RF_GRAY);
            rf_draw_text(TextFormat("HI-SCORE: %04i", hiScore), 20, 70, 20, RF_LIGHTGRAY);

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