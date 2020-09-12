/*******************************************************************************************
*
*   raylib - sample game: asteroids survival
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
#define PLAYER_BASE_SIZE    20.0f
#define PLAYER_SPEED        6.0f
#define PLAYER_MAX_SHOOTS   10

#define METEORS_SPEED       2
#define MAX_MEDIUM_METEORS  8
#define MAX_SMALL_METEORS   16

//----------------------------------------------------------------------------------
// Types and Structures Definition
//----------------------------------------------------------------------------------
typedef struct Player {
    rf_vec2 position;
    rf_vec2 speed;
    float acceleration;
    float rotation;
    rf_vec3 collider;
    rf_color color;
} Player;

typedef struct Meteor {
    rf_vec2 position;
    rf_vec2 speed;
    float radius;
    bool active;
    rf_color color;
} Meteor;

//------------------------------------------------------------------------------------
// Global Variables Declaration
//------------------------------------------------------------------------------------
static int framesCounter = 0;
static bool gameOver = false;
static bool pause = false;

// NOTE: Defined triangle is isosceles with common angles of 70 degrees.
static float shipHeight = 0.0f;

static Player player = { 0 };
static Meteor mediumMeteor[MAX_MEDIUM_METEORS] = { 0 };
static Meteor smallMeteor[MAX_SMALL_METEORS] = { 0 };

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
    .title  = "sample game: asteroids survival"
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
    int posx, posy;
    int velx, vely;
    bool correctRange = false;

    pause = false;

    framesCounter = 0;

    shipHeight = (PLAYER_BASE_SIZE / 2) / tanf(20 * RF_DEG2RAD);

    // Initialization player
    player.position = (rf_vec2){ window.width / 2, window.height / 2 - shipHeight / 2 };
    player.speed = (rf_vec2){ 0, 0 };
    player.acceleration = 0;
    player.rotation = 0;
    player.collider = (rf_vec3){ player.position.x + sin(player.rotation * RF_DEG2RAD) * (shipHeight / 2.5f), player.position.y - cos(player.rotation * RF_DEG2RAD) * (shipHeight / 2.5f), 12 };
    player.color = RF_LIGHTGRAY;

    for (int i = 0; i < MAX_MEDIUM_METEORS; i++)
    {
        posx = GetRandomValue(0, window.width);

        while (!correctRange)
        {
            if (posx > window.width / 2 - 150 && posx < window.width / 2 + 150) posx = GetRandomValue(0, window.width);
            else correctRange = true;
        }

        correctRange = false;

        posy = GetRandomValue(0, window.height);

        while (!correctRange)
        {
            if (posy > window.height / 2 - 150 && posy < window.height / 2 + 150)  posy = GetRandomValue(0, window.height);
            else correctRange = true;
        }

        correctRange = false;
        velx = GetRandomValue(-METEORS_SPEED, METEORS_SPEED);
        vely = GetRandomValue(-METEORS_SPEED, METEORS_SPEED);

        while (!correctRange)
        {
            if (velx == 0 && vely == 0)
            {
                velx = GetRandomValue(-METEORS_SPEED, METEORS_SPEED);
                vely = GetRandomValue(-METEORS_SPEED, METEORS_SPEED);
            }
            else correctRange = true;
        }
        mediumMeteor[i].position = (rf_vec2){ posx, posy };
        mediumMeteor[i].speed = (rf_vec2){ velx, vely };
        mediumMeteor[i].radius = 20;
        mediumMeteor[i].active = true;
        mediumMeteor[i].color = RF_GREEN;
    }

    for (int i = 0; i < MAX_SMALL_METEORS; i++)
    {
        posx = GetRandomValue(0, window.width);

        while (!correctRange)
        {
            if (posx > window.width / 2 - 150 && posx < window.width / 2 + 150) posx = GetRandomValue(0, window.width);
            else correctRange = true;
        }

        correctRange = false;

        posy = GetRandomValue(0, window.height);

        while (!correctRange)
        {
            if (posy > window.height / 2 - 150 && posy < window.height / 2 + 150)  posy = GetRandomValue(0, window.height);
            else correctRange = true;
        }

        correctRange = false;
        velx = GetRandomValue(-METEORS_SPEED, METEORS_SPEED);
        vely = GetRandomValue(-METEORS_SPEED, METEORS_SPEED);

        while (!correctRange)
        {
            if (velx == 0 && vely == 0)
            {
                velx = GetRandomValue(-METEORS_SPEED, METEORS_SPEED);
                vely = GetRandomValue(-METEORS_SPEED, METEORS_SPEED);
            }
            else correctRange = true;
        }
        smallMeteor[i].position = (rf_vec2){ posx, posy };
        smallMeteor[i].speed = (rf_vec2){ velx, vely };
        smallMeteor[i].radius = 10;
        smallMeteor[i].active = true;
        smallMeteor[i].color = RF_YELLOW;
    }
}

// Update game (one frame)
void UpdateGame(const platform_input_state* input)
{
    if (!gameOver)
    {
        if (input->keys[KEYCODE_P] == KEY_HOLD_DOWN) pause = !pause;

        if (!pause)
        {
            framesCounter++;

            // Player logic

            // Rotation
            if (input->keys[KEYCODE_LEFT] == KEY_HOLD_DOWN) player.rotation -= 5;
            if (input->keys[KEYCODE_RIGHT] == KEY_HOLD_DOWN) player.rotation += 5;

            // Speed
            player.speed.x = sin(player.rotation * RF_DEG2RAD) * PLAYER_SPEED;
            player.speed.y = cos(player.rotation * RF_DEG2RAD) * PLAYER_SPEED;

            // Controller
            if (input->keys[KEYCODE_UP] == KEY_HOLD_DOWN)
            {
                if (player.acceleration < 1) player.acceleration += 0.04f;
            }
            else
            {
                if (player.acceleration > 0) player.acceleration -= 0.02f;
                else if (player.acceleration < 0) player.acceleration = 0;
            }
            if (input->keys[KEYCODE_DOWN] == KEY_HOLD_DOWN)
            {
                if (player.acceleration > 0) player.acceleration -= 0.04f;
                else if (player.acceleration < 0) player.acceleration = 0;
            }

            // Movement
            player.position.x += (player.speed.x * player.acceleration);
            player.position.y -= (player.speed.y * player.acceleration);

            // Wall behaviour for player
            if (player.position.x > window.width + shipHeight) player.position.x = -(shipHeight);
            else if (player.position.x < -(shipHeight)) player.position.x = window.width + shipHeight;
            if (player.position.y > (window.height + shipHeight)) player.position.y = -(shipHeight);
            else if (player.position.y < -(shipHeight)) player.position.y = window.height + shipHeight;

            // Collision Player to meteors
            player.collider = (rf_vec3){ player.position.x + sin(player.rotation * RF_DEG2RAD) * (shipHeight / 2.5f), player.position.y - cos(player.rotation * RF_DEG2RAD) * (shipHeight / 2.5f), 12 };

            for (int a = 0; a < MAX_MEDIUM_METEORS; a++)
            {
                if (rf_check_collision_circles((rf_vec2) { player.collider.x, player.collider.y }, player.collider.z, mediumMeteor[a].position, mediumMeteor[a].radius) && mediumMeteor[a].active) gameOver = true;
            }

            for (int a = 0; a < MAX_SMALL_METEORS; a++)
            {
                if (rf_check_collision_circles((rf_vec2) { player.collider.x, player.collider.y }, player.collider.z, smallMeteor[a].position, smallMeteor[a].radius) && smallMeteor[a].active) gameOver = true;
            }

            // Meteor logic

            for (int i = 0; i < MAX_MEDIUM_METEORS; i++)
            {
                if (mediumMeteor[i].active)
                {
                    // movement
                    mediumMeteor[i].position.x += mediumMeteor[i].speed.x;
                    mediumMeteor[i].position.y += mediumMeteor[i].speed.y;

                    // wall behaviour
                    if (mediumMeteor[i].position.x > window.width + mediumMeteor[i].radius) mediumMeteor[i].position.x = -(mediumMeteor[i].radius);
                    else if (mediumMeteor[i].position.x < 0 - mediumMeteor[i].radius) mediumMeteor[i].position.x = window.width + mediumMeteor[i].radius;
                    if (mediumMeteor[i].position.y > window.height + mediumMeteor[i].radius) mediumMeteor[i].position.y = -(mediumMeteor[i].radius);
                    else if (mediumMeteor[i].position.y < 0 - mediumMeteor[i].radius) mediumMeteor[i].position.y = window.height + mediumMeteor[i].radius;
                }
            }

            for (int i = 0; i < MAX_SMALL_METEORS; i++)
            {
                if (smallMeteor[i].active)
                {
                    // movement
                    smallMeteor[i].position.x += smallMeteor[i].speed.x;
                    smallMeteor[i].position.y += smallMeteor[i].speed.y;

                    // wall behaviour
                    if (smallMeteor[i].position.x > window.width + smallMeteor[i].radius) smallMeteor[i].position.x = -(smallMeteor[i].radius);
                    else if (smallMeteor[i].position.x < 0 - smallMeteor[i].radius) smallMeteor[i].position.x = window.width + smallMeteor[i].radius;
                    if (smallMeteor[i].position.y > window.height + smallMeteor[i].radius) smallMeteor[i].position.y = -(smallMeteor[i].radius);
                    else if (smallMeteor[i].position.y < 0 - smallMeteor[i].radius) smallMeteor[i].position.y = window.height + smallMeteor[i].radius;
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
            // Draw spaceship
            rf_vec2 v1 = { player.position.x + sinf(player.rotation * RF_DEG2RAD) * (shipHeight), player.position.y - cosf(player.rotation * RF_DEG2RAD) * (shipHeight) };
            rf_vec2 v2 = { player.position.x - cosf(player.rotation * RF_DEG2RAD) * (PLAYER_BASE_SIZE / 2), player.position.y - sinf(player.rotation * RF_DEG2RAD) * (PLAYER_BASE_SIZE / 2) };
            rf_vec2 v3 = { player.position.x + cosf(player.rotation * RF_DEG2RAD) * (PLAYER_BASE_SIZE / 2), player.position.y + sinf(player.rotation * RF_DEG2RAD) * (PLAYER_BASE_SIZE / 2) };
            rf_draw_triangle(v1, v2, v3, RF_MAROON);

            // Draw meteor
            for (int i = 0; i < MAX_MEDIUM_METEORS; i++)
            {
                if (mediumMeteor[i].active) rf_draw_circle_v(mediumMeteor[i].position, mediumMeteor[i].radius, RF_GRAY);
                else rf_draw_circle_v(mediumMeteor[i].position, mediumMeteor[i].radius, rf_fade(RF_LIGHTGRAY, 0.3f));
            }

            for (int i = 0; i < MAX_SMALL_METEORS; i++)
            {
                if (smallMeteor[i].active) rf_draw_circle_v(smallMeteor[i].position, smallMeteor[i].radius, RF_DARKGRAY);
                else rf_draw_circle_v(smallMeteor[i].position, smallMeteor[i].radius, rf_fade(RF_LIGHTGRAY, 0.3f));
            }

            rf_draw_text(TextFormat("TIME: %.02f", (float)framesCounter / 60), 10, 10, 20, RF_BLACK);

            if (pause) rf_draw_text("GAME PAUSED", window.width / 2 - rf_measure_text(rf_get_default_font(), "GAME PAUSED", 40, 0).width / 2, window.height / 2 - 40, 40, RF_GRAY);
        }
        else rf_draw_text("PRESS [ENTER] TO PLAY AGAIN", window.width / 2 - rf_measure_text(rf_get_default_font(), "PRESS [ENTER] TO PLAY AGAIN", 20, 0).width / 2, window.height / 2 - 50, 20, RF_GRAY);
    }
    rf_end();
    //----------------------------------------------------------------------------------
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