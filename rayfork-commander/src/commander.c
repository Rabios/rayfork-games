/*******************************************************************************************
*
*   raylib - sample game: missile commander
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
#define MAX_MISSILES                100
#define MAX_INTERCEPTORS            30
#define MAX_EXPLOSIONS              100
#define LAUNCHERS_AMOUNT            3           // Not a variable, should not be changed
#define BUILDINGS_AMOUNT            6           // Not a variable, should not be changed

#define LAUNCHER_SIZE               80
#define BUILDING_SIZE               60
#define EXPLOSION_RADIUS            40

#define MISSILE_SPEED               1
#define MISSILE_LAUNCH_FRAMES       80
#define INTERCEPTOR_SPEED           10
#define EXPLOSION_INCREASE_TIME     90          // In frames
#define EXPLOSION_TOTAL_TIME        210         // In frames

#define EXPLOSION_COLOR             (rf_color){ 125, 125, 125, 125 }

//----------------------------------------------------------------------------------
// Types and Structures Definition
//----------------------------------------------------------------------------------
typedef struct Missile {
    rf_vec2 origin;
    rf_vec2 position;
    rf_vec2 objective;
    rf_vec2 speed;

    bool active;
} Missile;

typedef struct Interceptor {
    rf_vec2 origin;
    rf_vec2 position;
    rf_vec2 objective;
    rf_vec2 speed;

    bool active;
} Interceptor;

typedef struct Explosion {
    rf_vec2 position;
    float radiusMultiplier;
    int frame;
    bool active;
} Explosion;

typedef struct Launcher {
    rf_vec2 position;
    bool active;
} Launcher;

typedef struct Building {
    rf_vec2 position;
    bool active;
} Building;

//------------------------------------------------------------------------------------
// Global Variables Declaration
//------------------------------------------------------------------------------------
static int framesCounter = 0;
static bool gameOver = false;
static bool pause = false;
static int score = 0;

static Missile missile[MAX_MISSILES] = { 0 };
static Interceptor interceptor[MAX_INTERCEPTORS] = { 0 };
static Explosion explosion[MAX_EXPLOSIONS] = { 0 };
static Launcher launcher[LAUNCHERS_AMOUNT] = { 0 };
static Building building[BUILDINGS_AMOUNT] = { 0 };
static int explosionIndex = 0;

//------------------------------------------------------------------------------------
// Module Functions Declaration (local)
//------------------------------------------------------------------------------------
static void InitGame(void);                                      // Initialize game
static void UpdateGame(const platform_input_state* input);       // Update game (one frame)
static void DrawGame(void);                                      // Draw game (one frame)
static void UnloadGame(void);                                    // Unload game
static void UpdateDrawFrame(const platform_input_state* input);  // Update and Draw (one frame)

// Additional module functions
static void UpdateOutgoingFire(const platform_input_state* input);
static void UpdateIncomingFire(const platform_input_state* input);

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
    .title  ="sample game: missile commander"
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

//--------------------------------------------------------------------------------------
// Game Module Functions Definition
//--------------------------------------------------------------------------------------

// Initialize game variables
void InitGame()
{
    // Initialize missiles
    for (int i = 0; i < MAX_MISSILES; i++)
    {
        missile[i].origin = (rf_vec2){ 0, 0 };
        missile[i].speed = (rf_vec2){ 0, 0 };
        missile[i].position = (rf_vec2){ 0, 0 };

        missile[i].active = false;
    }

    // Initialize interceptors
    for (int i = 0; i < MAX_INTERCEPTORS; i++)
    {
        interceptor[i].origin = (rf_vec2){ 0, 0 };
        interceptor[i].speed = (rf_vec2){ 0, 0 };
        interceptor[i].position = (rf_vec2){ 0, 0 };

        interceptor[i].active = false;
    }

    // Initialize explosions
    for (int i = 0; i < MAX_EXPLOSIONS; i++)
    {
        explosion[i].position = (rf_vec2){ 0, 0 };
        explosion[i].frame = 0;
        explosion[i].active = false;
    }

    // Initialize buildings and launchers
    int sparcing = window.width / (LAUNCHERS_AMOUNT + BUILDINGS_AMOUNT + 1);

    // Buildings and launchers placing
    launcher[0].position = (rf_vec2){ 1 * sparcing, window.height - LAUNCHER_SIZE / 2 };
    building[0].position = (rf_vec2){ 2 * sparcing, window.height - BUILDING_SIZE / 2 };
    building[1].position = (rf_vec2){ 3 * sparcing, window.height - BUILDING_SIZE / 2 };
    building[2].position = (rf_vec2){ 4 * sparcing, window.height - BUILDING_SIZE / 2 };
    launcher[1].position = (rf_vec2){ 5 * sparcing, window.height - LAUNCHER_SIZE / 2 };
    building[3].position = (rf_vec2){ 6 * sparcing, window.height - BUILDING_SIZE / 2 };
    building[4].position = (rf_vec2){ 7 * sparcing, window.height - BUILDING_SIZE / 2 };
    building[5].position = (rf_vec2){ 8 * sparcing, window.height - BUILDING_SIZE / 2 };
    launcher[2].position = (rf_vec2){ 9 * sparcing, window.height - LAUNCHER_SIZE / 2 };

    // Buildings and launchers activation
    for (int i = 0; i < LAUNCHERS_AMOUNT; i++) launcher[i].active = true;
    for (int i = 0; i < BUILDINGS_AMOUNT; i++) building[i].active = true;

    // Initialize game variables
    score = 0;
}

// Update game (one frame)
void UpdateGame(const platform_input_state* input)
{
    if (!gameOver)
    {
        if (input->keys[KEYCODE_P] == KEY_PRESSED_DOWN) pause = !pause;

        if (!pause)
        {
            framesCounter++;

            static
                float distance;

            // Interceptors update
            for (int i = 0; i < MAX_INTERCEPTORS; i++)
            {
                if (interceptor[i].active)
                {
                    // Update position
                    interceptor[i].position.x += interceptor[i].speed.x;
                    interceptor[i].position.y += interceptor[i].speed.y;

                    // Distance to objective
                    distance = sqrt(pow(interceptor[i].position.x - interceptor[i].objective.x, 2) +
                        pow(interceptor[i].position.y - interceptor[i].objective.y, 2));

                    if (distance < INTERCEPTOR_SPEED)
                    {
                        // Interceptor dissapears
                        interceptor[i].active = false;

                        // Explosion
                        explosion[explosionIndex].position = interceptor[i].position;
                        explosion[explosionIndex].active = true;
                        explosion[explosionIndex].frame = 0;
                        explosionIndex++;
                        if (explosionIndex == MAX_EXPLOSIONS) explosionIndex = 0;

                        break;
                    }
                }
            }

            // Missiles update
            for (int i = 0; i < MAX_MISSILES; i++)
            {
                if (missile[i].active)
                {
                    // Update position
                    missile[i].position.x += missile[i].speed.x;
                    missile[i].position.y += missile[i].speed.y;

                    // Collision and missile out of bounds
                    if (missile[i].position.y > window.height) missile[i].active = false;
                    else
                    {
                        // CHeck collision with launchers
                        for (int j = 0; j < LAUNCHERS_AMOUNT; j++)
                        {
                            if (launcher[j].active)
                            {
                                if (rf_check_collision_point_rec(missile[i].position, (rf_rec) {
                                    launcher[j].position.x - LAUNCHER_SIZE / 2, launcher[j].position.y - LAUNCHER_SIZE / 2,
                                        LAUNCHER_SIZE, LAUNCHER_SIZE
                                }))
                                {
                                    // Missile dissapears
                                    missile[i].active = false;

                                    // Explosion and destroy building
                                    launcher[j].active = false;

                                    explosion[explosionIndex].position = missile[i].position;
                                    explosion[explosionIndex].active = true;
                                    explosion[explosionIndex].frame = 0;
                                    explosionIndex++;
                                    if (explosionIndex == MAX_EXPLOSIONS) explosionIndex = 0;

                                    break;
                                }
                            }
                        }

                        // CHeck collision with buildings
                        for (int j = 0; j < BUILDINGS_AMOUNT; j++)
                        {
                            if (building[j].active)
                            {
                                if (rf_check_collision_point_rec(missile[i].position, (rf_rec) {
                                    building[j].position.x - BUILDING_SIZE / 2, building[j].position.y - BUILDING_SIZE / 2,
                                        BUILDING_SIZE, BUILDING_SIZE
                                }))
                                {
                                    // Missile dissapears
                                    missile[i].active = false;

                                    // Explosion and destroy building
                                    building[j].active = false;

                                    explosion[explosionIndex].position = missile[i].position;
                                    explosion[explosionIndex].active = true;
                                    explosion[explosionIndex].frame = 0;
                                    explosionIndex++;
                                    if (explosionIndex == MAX_EXPLOSIONS) explosionIndex = 0;

                                    break;
                                }
                            }
                        }

                        // CHeck collision with explosions
                        for (int j = 0; j < MAX_EXPLOSIONS; j++)
                        {
                            if (explosion[j].active)
                            {
                                if (rf_check_collision_point_circle(missile[i].position, explosion[j].position, EXPLOSION_RADIUS * explosion[j].radiusMultiplier))
                                {
                                    // Missile dissapears and we earn 100 points
                                    missile[i].active = false;
                                    score += 100;

                                    explosion[explosionIndex].position = missile[i].position;
                                    explosion[explosionIndex].active = true;
                                    explosion[explosionIndex].frame = 0;
                                    explosionIndex++;
                                    if (explosionIndex == MAX_EXPLOSIONS) explosionIndex = 0;

                                    break;
                                }
                            }
                        }
                    }
                }
            }

            // Explosions update
            for (int i = 0; i < MAX_EXPLOSIONS; i++)
            {
                if (explosion[i].active)
                {
                    explosion[i].frame++;

                    if (explosion[i].frame <= EXPLOSION_INCREASE_TIME) explosion[i].radiusMultiplier = explosion[i].frame / (float)EXPLOSION_INCREASE_TIME;
                    else if (explosion[i].frame <= EXPLOSION_TOTAL_TIME) explosion[i].radiusMultiplier = 1 - (explosion[i].frame - (float)EXPLOSION_INCREASE_TIME) / (float)EXPLOSION_TOTAL_TIME;
                    else
                    {
                        explosion[i].frame = 0;
                        explosion[i].active = false;
                    }
                }
            }

            // Fire logic
            UpdateOutgoingFire(input);
            UpdateIncomingFire(input);

            // Game over logic
            int checker = 0;

            for (int i = 0; i < LAUNCHERS_AMOUNT; i++)
            {
                if (!launcher[i].active) checker++;
                if (checker == LAUNCHERS_AMOUNT) gameOver = true;
            }

            checker = 0;
            for (int i = 0; i < BUILDINGS_AMOUNT; i++)
            {
                if (!building[i].active) checker++;
                if (checker == BUILDINGS_AMOUNT) gameOver = true;
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
            // Draw missiles
            for (int i = 0; i < MAX_MISSILES; i++)
            {
                if (missile[i].active)
                {
                    rf_draw_line(missile[i].origin.x, missile[i].origin.y, missile[i].position.x, missile[i].position.y, RF_RED);

                    if (framesCounter % 16 < 8) rf_draw_circle(missile[i].position.x, missile[i].position.y, 3, RF_YELLOW);
                }
            }

            // Draw interceptors
            for (int i = 0; i < MAX_INTERCEPTORS; i++)
            {
                if (interceptor[i].active)
                {
                    rf_draw_line(interceptor[i].origin.x, interceptor[i].origin.y, interceptor[i].position.x, interceptor[i].position.y, RF_GREEN);

                    if (framesCounter % 16 < 8) rf_draw_circle(interceptor[i].position.x, interceptor[i].position.y, 3, RF_BLUE);
                }
            }

            // Draw explosions
            for (int i = 0; i < MAX_EXPLOSIONS; i++)
            {
                if (explosion[i].active) rf_draw_circle(explosion[i].position.x, explosion[i].position.y, EXPLOSION_RADIUS * explosion[i].radiusMultiplier, EXPLOSION_COLOR);
            }

            // Draw buildings and launchers
            for (int i = 0; i < LAUNCHERS_AMOUNT; i++)
            {
                if (launcher[i].active) rf_draw_rectangle(launcher[i].position.x - LAUNCHER_SIZE / 2, launcher[i].position.y - LAUNCHER_SIZE / 2, LAUNCHER_SIZE, LAUNCHER_SIZE, RF_GRAY);
            }

            for (int i = 0; i < BUILDINGS_AMOUNT; i++)
            {
                if (building[i].active) rf_draw_rectangle(building[i].position.x - BUILDING_SIZE / 2, building[i].position.y - BUILDING_SIZE / 2, BUILDING_SIZE, BUILDING_SIZE, RF_LIGHTGRAY);
            }

            // Draw score
            rf_draw_text(TextFormat("SCORE %4i", score), 20, 20, 40, RF_LIGHTGRAY);

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

//--------------------------------------------------------------------------------------
// Additional module functions
//--------------------------------------------------------------------------------------
static void UpdateOutgoingFire(const platform_input_state* input)
{
    static int interceptorNumber = 0;
    int launcherShooting = 0;

    if (input->left_mouse_btn == BTN_PRESSED_DOWN) launcherShooting = 1;
    if (input->middle_mouse_btn == BTN_PRESSED_DOWN) launcherShooting = 2;
    if (input->mouse_y == BTN_PRESSED_DOWN) launcherShooting = 3;

    if (launcherShooting > 0 && launcher[launcherShooting - 1].active)
    {
        float module;
        float sideX;
        float sideY;

        // Activate the interceptor
        interceptor[interceptorNumber].active = true;

        // Assign start position
        interceptor[interceptorNumber].origin = launcher[launcherShooting - 1].position;
        interceptor[interceptorNumber].position = interceptor[interceptorNumber].origin;
        interceptor[interceptorNumber].objective = (rf_vec2){ input->mouse_x, input->mouse_y };

        // Calculate speed
        module = sqrt(pow(interceptor[interceptorNumber].objective.x - interceptor[interceptorNumber].origin.x, 2) +
            pow(interceptor[interceptorNumber].objective.y - interceptor[interceptorNumber].origin.y, 2));

        sideX = (interceptor[interceptorNumber].objective.x - interceptor[interceptorNumber].origin.x) * INTERCEPTOR_SPEED / module;
        sideY = (interceptor[interceptorNumber].objective.y - interceptor[interceptorNumber].origin.y) * INTERCEPTOR_SPEED / module;

        interceptor[interceptorNumber].speed = (rf_vec2){ sideX, sideY };

        // Update
        interceptorNumber++;
        if (interceptorNumber == MAX_INTERCEPTORS) interceptorNumber = 0;
    }
}

static void UpdateIncomingFire(const platform_input_state* input)
{
    static int missileIndex = 0;

    // Launch missile
    if (framesCounter % MISSILE_LAUNCH_FRAMES == 0)
    {
        float module;
        float sideX;
        float sideY;

        // Activate the missile
        missile[missileIndex].active = true;

        // Assign start position
        missile[missileIndex].origin = (rf_vec2){ GetRandomValue(20, window.width - 20), -10 };
        missile[missileIndex].position = missile[missileIndex].origin;
        missile[missileIndex].objective = (rf_vec2){ GetRandomValue(20, window.width - 20), window.height + 10 };

        // Calculate speed
        module = sqrt(pow(missile[missileIndex].objective.x - missile[missileIndex].origin.x, 2) +
            pow(missile[missileIndex].objective.y - missile[missileIndex].origin.y, 2));

        sideX = (missile[missileIndex].objective.x - missile[missileIndex].origin.x) * MISSILE_SPEED / module;
        sideY = (missile[missileIndex].objective.y - missile[missileIndex].origin.y) * MISSILE_SPEED / module;

        missile[missileIndex].speed = (rf_vec2){ sideX, sideY };

        // Update
        missileIndex++;
        if (missileIndex == MAX_MISSILES) missileIndex = 0;
    }
}