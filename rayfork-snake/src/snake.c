/*******************************************************************************************
*
*   raylib - sample game: snake
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
#define SNAKE_LENGTH   256
#define SQUARE_SIZE     31

//----------------------------------------------------------------------------------
// Types and Structures Definition
//----------------------------------------------------------------------------------
typedef struct Snake {
    rf_vec2 position;
    rf_vec2 size;
    rf_vec2 speed;
    rf_color color;
} Snake;

typedef struct Food {
    rf_vec2 position;
    rf_vec2 size;
    bool active;
    rf_color color;
} Food;

//------------------------------------------------------------------------------------
// Global Variables Declaration
//------------------------------------------------------------------------------------
static int framesCounter = 0;
static bool gameOver = false;
static bool pause = false;

static Food fruit = { 0 };
static Snake snake[SNAKE_LENGTH] = { 0 };
static rf_vec2 snakePosition[SNAKE_LENGTH] = { 0 };
static bool allowMove = false;
static rf_vec2 offset = { 0 };
static int counterTail = 0;

//------------------------------------------------------------------------------------
// Module Functions Declaration (local)
//------------------------------------------------------------------------------------
static void InitGame(void);                                      // Initialize game
static void UpdateGame(const platform_input_state* input);       // Update game (one frame)
static void DrawGame(void);                                      // Draw game (one frame)
static void UnloadGame(void);                                    // Unload game
static void UpdateDrawFrame(const platform_input_state* input);  // Update and Draw (one frame)

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
    .title  = "sample game: snake"
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
    framesCounter = 0;
    gameOver = false;
    pause = false;

    counterTail = 1;
    allowMove = false;

    offset.x = window.width % SQUARE_SIZE;
    offset.y = window.height % SQUARE_SIZE;

    for (int i = 0; i < SNAKE_LENGTH; i++)
    {
        snake[i].position = (rf_vec2){ offset.x / 2, offset.y / 2 };
        snake[i].size = (rf_vec2){ SQUARE_SIZE, SQUARE_SIZE };
        snake[i].speed = (rf_vec2){ SQUARE_SIZE, 0 };

        if (i == 0) snake[i].color = RF_DARKBLUE;
        else snake[i].color = RF_BLUE;
    }

    for (int i = 0; i < SNAKE_LENGTH; i++)
    {
        snakePosition[i] = (rf_vec2){ 0.0f, 0.0f };
    }

    fruit.size = (rf_vec2){ SQUARE_SIZE, SQUARE_SIZE };
    fruit.color = RF_SKYBLUE;
    fruit.active = false;
}

// Update game (one frame)
void UpdateGame(const platform_input_state* input)
{
    if (!gameOver)
    {
        if (input->keys[KEYCODE_P] == KEY_PRESSED_DOWN) pause = !pause;

        if (!pause)
        {
            // Player control
            if (input->keys[KEYCODE_RIGHT] == KEY_PRESSED_DOWN && (snake[0].speed.x == 0) && allowMove)
            {
                snake[0].speed = (rf_vec2){ SQUARE_SIZE, 0 };
                allowMove = false;
            }
            if (input->keys[KEYCODE_LEFT] == KEY_PRESSED_DOWN && (snake[0].speed.x == 0) && allowMove)
            {
                snake[0].speed = (rf_vec2){ -SQUARE_SIZE, 0 };
                allowMove = false;
            }
            if (input->keys[KEYCODE_UP] == KEY_PRESSED_DOWN && (snake[0].speed.y == 0) && allowMove)
            {
                snake[0].speed = (rf_vec2){ 0, -SQUARE_SIZE };
                allowMove = false;
            }
            if (input->keys[KEYCODE_DOWN] == KEY_PRESSED_DOWN && (snake[0].speed.y == 0) && allowMove)
            {
                snake[0].speed = (rf_vec2){ 0, SQUARE_SIZE };
                allowMove = false;
            }

            // Snake movement
            for (int i = 0; i < counterTail; i++) snakePosition[i] = snake[i].position;

            if ((framesCounter % 5) == 0)
            {
                for (int i = 0; i < counterTail; i++)
                {
                    if (i == 0)
                    {
                        snake[0].position.x += snake[0].speed.x;
                        snake[0].position.y += snake[0].speed.y;
                        allowMove = true;
                    }
                    else snake[i].position = snakePosition[i - 1];
                }
            }

            // Wall behaviour
            if (((snake[0].position.x) > (window.width - offset.x)) ||
                ((snake[0].position.y) > (window.height - offset.y)) ||
                (snake[0].position.x < 0) || (snake[0].position.y < 0))
            {
                gameOver = true;
            }

            // Collision with yourself
            for (int i = 1; i < counterTail; i++)
            {
                if ((snake[0].position.x == snake[i].position.x) && (snake[0].position.y == snake[i].position.y)) gameOver = true;
            }

            // Fruit position calculation
            if (!fruit.active)
            {
                fruit.active = true;
                fruit.position = (rf_vec2){ GetRandomValue(0, (window.width / SQUARE_SIZE) - 1) * SQUARE_SIZE + offset.x / 2, GetRandomValue(0, (window.height / SQUARE_SIZE) - 1) * SQUARE_SIZE + offset.y / 2 };

                for (int i = 0; i < counterTail; i++)
                {
                    while ((fruit.position.x == snake[i].position.x) && (fruit.position.y == snake[i].position.y))
                    {
                        fruit.position = (rf_vec2){ GetRandomValue(0, (window.width / SQUARE_SIZE) - 1) * SQUARE_SIZE + offset.x / 2, GetRandomValue(0, (window.height / SQUARE_SIZE) - 1) * SQUARE_SIZE + offset.y / 2 };
                        i = 0;
                    }
                }
            }

            // Collision
            if ((snake[0].position.x < (fruit.position.x + fruit.size.x) && (snake[0].position.x + snake[0].size.x) > fruit.position.x) &&
                (snake[0].position.y < (fruit.position.y + fruit.size.y) && (snake[0].position.y + snake[0].size.y) > fruit.position.y))
            {
                snake[counterTail].position = snakePosition[counterTail - 1];
                counterTail += 1;
                fruit.active = false;
            }

            framesCounter++;
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
void DrawGame()
{
    rf_begin();
    {
        rf_clear(RF_RAYWHITE);

        if (!gameOver)
        {
            // Draw grid lines
            for (int i = 0; i < window.width / SQUARE_SIZE + 1; i++)
            {
                rf_draw_line_v((rf_vec2) { SQUARE_SIZE* i + offset.x / 2, offset.y / 2 }, (rf_vec2) { SQUARE_SIZE* i + offset.x / 2, window.height - offset.y / 2 }, RF_LIGHTGRAY);
            }

            for (int i = 0; i < window.width / SQUARE_SIZE + 1; i++)
            {
                rf_draw_line_v((rf_vec2) { offset.x / 2, SQUARE_SIZE* i + offset.y / 2 }, (rf_vec2) { window.width - offset.x / 2, SQUARE_SIZE* i + offset.y / 2 }, RF_LIGHTGRAY);
            }

            // Draw snake
            for (int i = 0; i < counterTail; i++) rf_draw_rectangle_v(snake[i].position, snake[i].size, snake[i].color);

            // Draw fruit to pick
            rf_draw_rectangle_v(fruit.position, fruit.size, fruit.color);

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