/*******************************************************************************************
*
*   raylib - sample game: arkanoid
*
*   Sample game Marc Palau and Ramon Santamaria
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
#define PLAYER_MAX_LIFE         5
#define LINES_OF_BRICKS         5
#define BRICKS_PER_LINE        20

//----------------------------------------------------------------------------------
// Types and Structures Definition
//----------------------------------------------------------------------------------
typedef enum GameScreen { LOGO, TITLE, GAMEPLAY, ENDING } GameScreen;

typedef struct Player {
    rf_vec2 position;
    rf_vec2 size;
    int life;
} Player;

typedef struct Ball {
    rf_vec2 position;
    rf_vec2 speed;
    int radius;
    bool active;
} Ball;

typedef struct Brick {
    rf_vec2 position;
    bool active;
} Brick;

//------------------------------------------------------------------------------------
// Global Variables Declaration
//------------------------------------------------------------------------------------
static bool gameOver = false;
static bool pause = false;

static Player player = { 0 };
static Ball ball = { 0 };
static Brick brick[LINES_OF_BRICKS][BRICKS_PER_LINE] = { 0 };
static rf_vec2 brickSize = { 0 };

//------------------------------------------------------------------------------------
// Module Functions Declaration (local)
//------------------------------------------------------------------------------------
static void InitGame(void);                                      // Initialize game
static void UpdateGame(const platform_input_state* input);       // Update game (one frame)
static void DrawGame(void);                                      // Draw game (one frame)
static void UnloadGame(void);                                    // Unload game
static void UpdateDrawFrame(const platform_input_state* input);  // Update and Draw (one frame)

//------------------------------------------------------------------------------------
// Program main entry point
//------------------------------------------------------------------------------------
platform_window_details window = {
    .width  = 800,
    .height = 450,
    .title  = "sample game: arkanoid"
};

rf_context ctx;
rf_render_batch batch;
const platform_input_state* input;

extern void game_init(rf_gfx_backend_data* gfx_data)
{
    // Initialization (Note windowTitle is unused on Android)
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
    brickSize = (rf_vec2){ window.width / BRICKS_PER_LINE, 40 };

    // Initialize player
    player.position = (rf_vec2){ window.width / 2, window.height * 7 / 8 };
    player.size = (rf_vec2){ window.width / 10, 20 };
    player.life = PLAYER_MAX_LIFE;

    // Initialize ball
    ball.position = (rf_vec2){ window.width / 2, window.height * 7 / 8 - 30 };
    ball.speed = (rf_vec2){ 0, 0 };
    ball.radius = 7;
    ball.active = false;

    // Initialize bricks
    int initialDownPosition = 50;

    for (int i = 0; i < LINES_OF_BRICKS; i++)
    {
        for (int j = 0; j < BRICKS_PER_LINE; j++)
        {
            brick[i][j].position = (rf_vec2){ j * brickSize.x + brickSize.x / 2, i * brickSize.y + initialDownPosition };
            brick[i][j].active = true;
        }
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
            // Player movement logic
            if (input->keys[KEYCODE_LEFT] == KEY_HOLD_DOWN) player.position.x -= 5;
            if ((player.position.x - player.size.x / 2) <= 0) player.position.x = player.size.x / 2;
            if (input->keys[KEYCODE_RIGHT] == KEY_HOLD_DOWN) player.position.x += 5;
            if ((player.position.x + player.size.x / 2) >= window.width) player.position.x = window.width - player.size.x / 2;

            // Ball launching logic
            if (!ball.active)
            {
                if (input->keys[KEYCODE_SPACE] == KEY_PRESSED_DOWN)
                {
                    ball.active = true;
                    ball.speed = (rf_vec2){ 0, -5 };
                }
            }

            // Ball movement logic
            if (ball.active)
            {
                ball.position.x += ball.speed.x;
                ball.position.y += ball.speed.y;
            }
            else
            {
                ball.position = (rf_vec2){ player.position.x, window.height * 7 / 8 - 30 };
            }

            // Collision logic: ball vs walls 
            if (((ball.position.x + ball.radius) >= window.width) || ((ball.position.x - ball.radius) <= 0)) ball.speed.x *= -1;
            if ((ball.position.y - ball.radius) <= 0) ball.speed.y *= -1;
            if ((ball.position.y + ball.radius) >= window.height)
            {
                ball.speed = (rf_vec2){ 0, 0 };
                ball.active = false;

                player.life--;
            }

            // Collision logic: ball vs player
            if (rf_check_collision_circle_rec(ball.position, ball.radius,
                (rf_rec) {
                player.position.x - player.size.x / 2, player.position.y - player.size.y / 2, player.size.x, player.size.y
            }))
            {
                if (ball.speed.y > 0)
                {
                    ball.speed.y *= -1;
                    ball.speed.x = (ball.position.x - player.position.x) / (player.size.x / 2) * 5;
                }
            }

                // Collision logic: ball vs bricks
                for (int i = 0; i < LINES_OF_BRICKS; i++)
                {
                    for (int j = 0; j < BRICKS_PER_LINE; j++)
                    {
                        if (brick[i][j].active)
                        {
                            // Hit below
                            if (((ball.position.y - ball.radius) <= (brick[i][j].position.y + brickSize.y / 2)) &&
                                ((ball.position.y - ball.radius) > (brick[i][j].position.y + brickSize.y / 2 + ball.speed.y)) &&
                                ((fabs(ball.position.x - brick[i][j].position.x)) < (brickSize.x / 2 + ball.radius * 2 / 3)) && (ball.speed.y < 0))
                            {
                                brick[i][j].active = false;
                                ball.speed.y *= -1;
                            }
                            // Hit above
                            else if (((ball.position.y + ball.radius) >= (brick[i][j].position.y - brickSize.y / 2)) &&
                                ((ball.position.y + ball.radius) < (brick[i][j].position.y - brickSize.y / 2 + ball.speed.y)) &&
                                ((fabs(ball.position.x - brick[i][j].position.x)) < (brickSize.x / 2 + ball.radius * 2 / 3)) && (ball.speed.y > 0))
                            {
                                brick[i][j].active = false;
                                ball.speed.y *= -1;
                            }
                            // Hit left
                            else if (((ball.position.x + ball.radius) >= (brick[i][j].position.x - brickSize.x / 2)) &&
                                ((ball.position.x + ball.radius) < (brick[i][j].position.x - brickSize.x / 2 + ball.speed.x)) &&
                                ((fabs(ball.position.y - brick[i][j].position.y)) < (brickSize.y / 2 + ball.radius * 2 / 3)) && (ball.speed.x > 0))
                            {
                                brick[i][j].active = false;
                                ball.speed.x *= -1;
                            }
                            // Hit right
                            else if (((ball.position.x - ball.radius) <= (brick[i][j].position.x + brickSize.x / 2)) &&
                                ((ball.position.x - ball.radius) > (brick[i][j].position.x + brickSize.x / 2 + ball.speed.x)) &&
                                ((fabs(ball.position.y - brick[i][j].position.y)) < (brickSize.y / 2 + ball.radius * 2 / 3)) && (ball.speed.x < 0))
                            {
                                brick[i][j].active = false;
                                ball.speed.x *= -1;
                            }
                        }
                    }
                }

                // Game over logic
                if (player.life <= 0) gameOver = true;
                else
                {
                    gameOver = true;

                    for (int i = 0; i < LINES_OF_BRICKS; i++)
                    {
                        for (int j = 0; j < BRICKS_PER_LINE; j++)
                        {
                            if (brick[i][j].active) gameOver = false;
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
            // Draw player bar
            rf_draw_rectangle(player.position.x - player.size.x / 2, player.position.y - player.size.y / 2, player.size.x, player.size.y, RF_BLACK);

            // Draw player lives
            for (int i = 0; i < player.life; i++) rf_draw_rectangle(20 + 40 * i, window.height - 30, 35, 10, RF_LIGHTGRAY);

            // Draw ball
            rf_draw_circle_v(ball.position, ball.radius, RF_MAROON);

            // Draw bricks
            for (int i = 0; i < LINES_OF_BRICKS; i++)
            {
                for (int j = 0; j < BRICKS_PER_LINE; j++)
                {
                    if (brick[i][j].active)
                    {
                        if ((i + j) % 2 == 0) rf_draw_rectangle(brick[i][j].position.x - brickSize.x / 2, brick[i][j].position.y - brickSize.y / 2, brickSize.x, brickSize.y, RF_GRAY);
                        else rf_draw_rectangle(brick[i][j].position.x - brickSize.x / 2, brick[i][j].position.y - brickSize.y / 2, brickSize.x, brickSize.y, RF_DARKGRAY);
                    }
                }
            }

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