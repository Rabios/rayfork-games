/*******************************************************************************************
*
*   raylib - sample game: asteroids
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
#define MAX_BIG_METEORS     4
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

typedef struct Shoot {
    rf_vec2 position;
    rf_vec2 speed;
    float radius;
    float rotation;
    int lifeSpawn;
    bool active;
    rf_color color;
} Shoot;

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
static bool gameOver = false;
static bool pause = false;
static bool victory = false;

// NOTE: Defined triangle is isosceles with common angles of 70 degrees.
static float shipHeight = 0.0f;

static Player player = { 0 };
static Shoot shoot[PLAYER_MAX_SHOOTS] = { 0 };
static Meteor bigMeteor[MAX_BIG_METEORS] = { 0 };
static Meteor mediumMeteor[MAX_MEDIUM_METEORS] = { 0 };
static Meteor smallMeteor[MAX_SMALL_METEORS] = { 0 };

static int midMeteorsCount = 0;
static int smallMeteorsCount = 0;
static int destroyedMeteorsCount = 0;

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
    .title  = "sample game: asteroids"
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
    victory = false;
    pause = false;

    shipHeight = (PLAYER_BASE_SIZE / 2) / tanf(20 * RF_DEG2RAD);

    // Initialization player
    player.position = (rf_vec2){ window.width / 2, window.height / 2 - shipHeight / 2 };
    player.speed = (rf_vec2){ 0, 0 };
    player.acceleration = 0;
    player.rotation = 0;
    player.collider = (rf_vec3){ player.position.x + sin(player.rotation * RF_DEG2RAD) * (shipHeight / 2.5f), player.position.y - cos(player.rotation * RF_DEG2RAD) * (shipHeight / 2.5f), 12 };
    player.color = RF_LIGHTGRAY;

    destroyedMeteorsCount = 0;

    // Initialization shoot
    for (int i = 0; i < PLAYER_MAX_SHOOTS; i++)
    {
        shoot[i].position = (rf_vec2){ 0, 0 };
        shoot[i].speed = (rf_vec2){ 0, 0 };
        shoot[i].radius = 2;
        shoot[i].active = false;
        shoot[i].lifeSpawn = 0;
        shoot[i].color = RF_WHITE;
    }

    for (int i = 0; i < MAX_BIG_METEORS; i++)
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

        bigMeteor[i].position = (rf_vec2){ posx, posy };

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

        bigMeteor[i].speed = (rf_vec2){ velx, vely };
        bigMeteor[i].radius = 40;
        bigMeteor[i].active = true;
        bigMeteor[i].color = RF_BLUE;
    }

    for (int i = 0; i < MAX_MEDIUM_METEORS; i++)
    {
        mediumMeteor[i].position = (rf_vec2){ -100, -100 };
        mediumMeteor[i].speed = (rf_vec2){ 0,0 };
        mediumMeteor[i].radius = 20;
        mediumMeteor[i].active = false;
        mediumMeteor[i].color = RF_BLUE;
    }

    for (int i = 0; i < MAX_SMALL_METEORS; i++)
    {
        smallMeteor[i].position = (rf_vec2){ -100, -100 };
        smallMeteor[i].speed = (rf_vec2){ 0,0 };
        smallMeteor[i].radius = 10;
        smallMeteor[i].active = false;
        smallMeteor[i].color = RF_BLUE;
    }

    midMeteorsCount = 0;
    smallMeteorsCount = 0;
}

// Update game (one frame)
void UpdateGame(const platform_input_state* input)
{
    if (!gameOver)
    {
        if (input->keys[KEYCODE_P] == KEY_PRESSED_DOWN) pause = !pause;

        if (!pause)
        {
            // Player logic: rotation
            if (input->keys[KEYCODE_LEFT] == KEY_HOLD_DOWN) player.rotation -= 5;
            if (input->keys[KEYCODE_RIGHT] == KEY_HOLD_DOWN) player.rotation += 5;

            // Player logic: speed
            player.speed.x = sin(player.rotation * RF_DEG2RAD) * PLAYER_SPEED;
            player.speed.y = cos(player.rotation * RF_DEG2RAD) * PLAYER_SPEED;

            // Player logic: acceleration
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

            // Player logic: movement
            player.position.x += (player.speed.x * player.acceleration);
            player.position.y -= (player.speed.y * player.acceleration);

            // Collision logic: player vs walls
            if (player.position.x > window.width + shipHeight) player.position.x = -(shipHeight);
            else if (player.position.x < -(shipHeight)) player.position.x = window.width + shipHeight;
            if (player.position.y > (window.height + shipHeight)) player.position.y = -(shipHeight);
            else if (player.position.y < -(shipHeight)) player.position.y = window.height + shipHeight;

            // Player shoot logic
            if (input->keys[KEYCODE_SPACE] == KEY_PRESSED_DOWN)
            {
                for (int i = 0; i < PLAYER_MAX_SHOOTS; i++)
                {
                    if (!shoot[i].active)
                    {
                        shoot[i].position = (rf_vec2){ player.position.x + sin(player.rotation * RF_DEG2RAD) * (shipHeight), player.position.y - cos(player.rotation * RF_DEG2RAD) * (shipHeight) };
                        shoot[i].active = true;
                        shoot[i].speed.x = 1.5 * sin(player.rotation * RF_DEG2RAD) * PLAYER_SPEED;
                        shoot[i].speed.y = 1.5 * cos(player.rotation * RF_DEG2RAD) * PLAYER_SPEED;
                        shoot[i].rotation = player.rotation;
                        break;
                    }
                }
            }

            // Shoot life timer
            for (int i = 0; i < PLAYER_MAX_SHOOTS; i++)
            {
                if (shoot[i].active) shoot[i].lifeSpawn++;
            }

            // Shot logic
            for (int i = 0; i < PLAYER_MAX_SHOOTS; i++)
            {
                if (shoot[i].active)
                {
                    // Movement
                    shoot[i].position.x += shoot[i].speed.x;
                    shoot[i].position.y -= shoot[i].speed.y;

                    // Collision logic: shoot vs walls
                    if (shoot[i].position.x > window.width + shoot[i].radius)
                    {
                        shoot[i].active = false;
                        shoot[i].lifeSpawn = 0;
                    }
                    else if (shoot[i].position.x < 0 - shoot[i].radius)
                    {
                        shoot[i].active = false;
                        shoot[i].lifeSpawn = 0;
                    }
                    if (shoot[i].position.y > window.height + shoot[i].radius)
                    {
                        shoot[i].active = false;
                        shoot[i].lifeSpawn = 0;
                    }
                    else if (shoot[i].position.y < 0 - shoot[i].radius)
                    {
                        shoot[i].active = false;
                        shoot[i].lifeSpawn = 0;
                    }

                    // Life of shoot
                    if (shoot[i].lifeSpawn >= 60)
                    {
                        shoot[i].position = (rf_vec2){ 0, 0 };
                        shoot[i].speed = (rf_vec2){ 0, 0 };
                        shoot[i].lifeSpawn = 0;
                        shoot[i].active = false;
                    }
                }
            }

            // Collision logic: player vs meteors
            player.collider = (rf_vec3){ player.position.x + sin(player.rotation * RF_DEG2RAD) * (shipHeight / 2.5f), player.position.y - cos(player.rotation * RF_DEG2RAD) * (shipHeight / 2.5f), 12 };

            for (int a = 0; a < MAX_BIG_METEORS; a++)
            {
                if (rf_check_collision_circles((rf_vec2) { player.collider.x, player.collider.y }, player.collider.z, bigMeteor[a].position, bigMeteor[a].radius) && bigMeteor[a].active) gameOver = true;
            }

            for (int a = 0; a < MAX_MEDIUM_METEORS; a++)
            {
                if (rf_check_collision_circles((rf_vec2) { player.collider.x, player.collider.y }, player.collider.z, mediumMeteor[a].position, mediumMeteor[a].radius) && mediumMeteor[a].active) gameOver = true;
            }

            for (int a = 0; a < MAX_SMALL_METEORS; a++)
            {
                if (rf_check_collision_circles((rf_vec2) { player.collider.x, player.collider.y }, player.collider.z, smallMeteor[a].position, smallMeteor[a].radius) && smallMeteor[a].active) gameOver = true;
            }

            // Meteors logic: big meteors
            for (int i = 0; i < MAX_BIG_METEORS; i++)
            {
                if (bigMeteor[i].active)
                {
                    // Movement
                    bigMeteor[i].position.x += bigMeteor[i].speed.x;
                    bigMeteor[i].position.y += bigMeteor[i].speed.y;

                    // Collision logic: meteor vs wall
                    if (bigMeteor[i].position.x > window.width + bigMeteor[i].radius) bigMeteor[i].position.x = -(bigMeteor[i].radius);
                    else if (bigMeteor[i].position.x < 0 - bigMeteor[i].radius) bigMeteor[i].position.x = window.width + bigMeteor[i].radius;
                    if (bigMeteor[i].position.y > window.height + bigMeteor[i].radius) bigMeteor[i].position.y = -(bigMeteor[i].radius);
                    else if (bigMeteor[i].position.y < 0 - bigMeteor[i].radius) bigMeteor[i].position.y = window.height + bigMeteor[i].radius;
                }
            }

            // Meteors logic: medium meteors
            for (int i = 0; i < MAX_MEDIUM_METEORS; i++)
            {
                if (mediumMeteor[i].active)
                {
                    // Movement
                    mediumMeteor[i].position.x += mediumMeteor[i].speed.x;
                    mediumMeteor[i].position.y += mediumMeteor[i].speed.y;

                    // Collision logic: meteor vs wall
                    if (mediumMeteor[i].position.x > window.width + mediumMeteor[i].radius) mediumMeteor[i].position.x = -(mediumMeteor[i].radius);
                    else if (mediumMeteor[i].position.x < 0 - mediumMeteor[i].radius) mediumMeteor[i].position.x = window.width + mediumMeteor[i].radius;
                    if (mediumMeteor[i].position.y > window.height + mediumMeteor[i].radius) mediumMeteor[i].position.y = -(mediumMeteor[i].radius);
                    else if (mediumMeteor[i].position.y < 0 - mediumMeteor[i].radius) mediumMeteor[i].position.y = window.height + mediumMeteor[i].radius;
                }
            }

            // Meteors logic: small meteors
            for (int i = 0; i < MAX_SMALL_METEORS; i++)
            {
                if (smallMeteor[i].active)
                {
                    // Movement
                    smallMeteor[i].position.x += smallMeteor[i].speed.x;
                    smallMeteor[i].position.y += smallMeteor[i].speed.y;

                    // Collision logic: meteor vs wall
                    if (smallMeteor[i].position.x > window.width + smallMeteor[i].radius) smallMeteor[i].position.x = -(smallMeteor[i].radius);
                    else if (smallMeteor[i].position.x < 0 - smallMeteor[i].radius) smallMeteor[i].position.x = window.width + smallMeteor[i].radius;
                    if (smallMeteor[i].position.y > window.height + smallMeteor[i].radius) smallMeteor[i].position.y = -(smallMeteor[i].radius);
                    else if (smallMeteor[i].position.y < 0 - smallMeteor[i].radius) smallMeteor[i].position.y = window.height + smallMeteor[i].radius;
                }
            }

            // Collision logic: player-shoots vs meteors
            for (int i = 0; i < PLAYER_MAX_SHOOTS; i++)
            {
                if ((shoot[i].active))
                {
                    for (int a = 0; a < MAX_BIG_METEORS; a++)
                    {
                        if (bigMeteor[a].active && rf_check_collision_circles(shoot[i].position, shoot[i].radius, bigMeteor[a].position, bigMeteor[a].radius))
                        {
                            shoot[i].active = false;
                            shoot[i].lifeSpawn = 0;
                            bigMeteor[a].active = false;
                            destroyedMeteorsCount++;

                            for (int j = 0; j < 2; j++)
                            {
                                if (midMeteorsCount % 2 == 0)
                                {
                                    mediumMeteor[midMeteorsCount].position = (rf_vec2){ bigMeteor[a].position.x, bigMeteor[a].position.y };
                                    mediumMeteor[midMeteorsCount].speed = (rf_vec2){ cos(shoot[i].rotation * RF_DEG2RAD) * METEORS_SPEED * -1, sin(shoot[i].rotation * RF_DEG2RAD) * METEORS_SPEED * -1 };
                                }
                                else
                                {
                                    mediumMeteor[midMeteorsCount].position = (rf_vec2){ bigMeteor[a].position.x, bigMeteor[a].position.y };
                                    mediumMeteor[midMeteorsCount].speed = (rf_vec2){ cos(shoot[i].rotation * RF_DEG2RAD) * METEORS_SPEED, sin(shoot[i].rotation * RF_DEG2RAD) * METEORS_SPEED };
                                }

                                mediumMeteor[midMeteorsCount].active = true;
                                midMeteorsCount++;
                            }
                            //bigMeteor[a].position = (rf_vec2){-100, -100};
                            bigMeteor[a].color = RF_RED;
                            a = MAX_BIG_METEORS;
                        }
                    }

                    for (int b = 0; b < MAX_MEDIUM_METEORS; b++)
                    {
                        if (mediumMeteor[b].active && rf_check_collision_circles(shoot[i].position, shoot[i].radius, mediumMeteor[b].position, mediumMeteor[b].radius))
                        {
                            shoot[i].active = false;
                            shoot[i].lifeSpawn = 0;
                            mediumMeteor[b].active = false;
                            destroyedMeteorsCount++;

                            for (int j = 0; j < 2; j++)
                            {
                                if (smallMeteorsCount % 2 == 0)
                                {
                                    smallMeteor[smallMeteorsCount].position = (rf_vec2){ mediumMeteor[b].position.x, mediumMeteor[b].position.y };
                                    smallMeteor[smallMeteorsCount].speed = (rf_vec2){ cos(shoot[i].rotation * RF_DEG2RAD) * METEORS_SPEED * -1, sin(shoot[i].rotation * RF_DEG2RAD) * METEORS_SPEED * -1 };
                                }
                                else
                                {
                                    smallMeteor[smallMeteorsCount].position = (rf_vec2){ mediumMeteor[b].position.x, mediumMeteor[b].position.y };
                                    smallMeteor[smallMeteorsCount].speed = (rf_vec2){ cos(shoot[i].rotation * RF_DEG2RAD) * METEORS_SPEED, sin(shoot[i].rotation * RF_DEG2RAD) * METEORS_SPEED };
                                }

                                smallMeteor[smallMeteorsCount].active = true;
                                smallMeteorsCount++;
                            }
                            //mediumMeteor[b].position = (rf_vec2){-100, -100};
                            mediumMeteor[b].color = RF_GREEN;
                            b = MAX_MEDIUM_METEORS;
                        }
                    }

                    for (int c = 0; c < MAX_SMALL_METEORS; c++)
                    {
                        if (smallMeteor[c].active && rf_check_collision_circles(shoot[i].position, shoot[i].radius, smallMeteor[c].position, smallMeteor[c].radius))
                        {
                            shoot[i].active = false;
                            shoot[i].lifeSpawn = 0;
                            smallMeteor[c].active = false;
                            destroyedMeteorsCount++;
                            smallMeteor[c].color = RF_YELLOW;
                            // smallMeteor[c].position = (rf_vec2){-100, -100};
                            c = MAX_SMALL_METEORS;
                        }
                    }
                }
            }
        }

        if (destroyedMeteorsCount == MAX_BIG_METEORS + MAX_MEDIUM_METEORS + MAX_SMALL_METEORS) victory = true;
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

            // Draw meteors
            for (int i = 0; i < MAX_BIG_METEORS; i++)
            {
                if (bigMeteor[i].active) rf_draw_circle_v(bigMeteor[i].position, bigMeteor[i].radius, RF_DARKGRAY);
                else rf_draw_circle_v(bigMeteor[i].position, bigMeteor[i].radius, rf_fade(RF_LIGHTGRAY, 0.3f));
            }

            for (int i = 0; i < MAX_MEDIUM_METEORS; i++)
            {
                if (mediumMeteor[i].active) rf_draw_circle_v(mediumMeteor[i].position, mediumMeteor[i].radius, RF_GRAY);
                else rf_draw_circle_v(mediumMeteor[i].position, mediumMeteor[i].radius, rf_fade(RF_LIGHTGRAY, 0.3f));
            }

            for (int i = 0; i < MAX_SMALL_METEORS; i++)
            {
                if (smallMeteor[i].active) rf_draw_circle_v(smallMeteor[i].position, smallMeteor[i].radius, RF_GRAY);
                else rf_draw_circle_v(smallMeteor[i].position, smallMeteor[i].radius, rf_fade(RF_LIGHTGRAY, 0.3f));
            }

            // Draw shoot
            for (int i = 0; i < PLAYER_MAX_SHOOTS; i++)
            {
                if (shoot[i].active) rf_draw_circle_v(shoot[i].position, shoot[i].radius, RF_BLACK);
            }

            if (victory) rf_draw_text("VICTORY", window.width / 2 - rf_measure_text(rf_get_default_font(), "VICTORY", 20, 0).width / 2, window.height / 2, 20, RF_LIGHTGRAY);

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