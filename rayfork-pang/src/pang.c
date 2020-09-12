/*******************************************************************************************
*
*   raylib - sample game: pang
*
*   Sample game developed by Ian Eito and Albert Martos and Ramon Santamaria
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
#define PLAYER_SPEED        5.0f
#define PLAYER_MAX_SHOOTS   1

#define MAX_BIG_BALLS       2
#define BALLS_SPEED         2.0f

//----------------------------------------------------------------------------------
// Types and Structures Definition
//----------------------------------------------------------------------------------
typedef struct Player {
    rf_vec2 position;
    rf_vec2 speed;
    rf_vec3 collider;
    float rotation;
} Player;

typedef struct Shoot {
    rf_vec2 position;
    rf_vec2 speed;
    float radius;
    float rotation;
    int lifeSpawn;
    bool active;
} Shoot;

typedef struct Ball {
    rf_vec2 position;
    rf_vec2 speed;
    float radius;
    int points;
    bool active;
} Ball;

typedef struct Points {
    rf_vec2 position;
    int value;
    float alpha;
} Points;

//------------------------------------------------------------------------------------
// Global Variables Declaration
//------------------------------------------------------------------------------------
static int framesCounter = 0;
static bool gameOver = false;
static bool pause = false;
static int score = 0;

static Player player = { 0 };
static Shoot shoot[PLAYER_MAX_SHOOTS] = { 0 };
static Ball bigBalls[MAX_BIG_BALLS] = { 0 };
static Ball mediumBalls[MAX_BIG_BALLS * 2] = { 0 };
static Ball smallBalls[MAX_BIG_BALLS * 4] = { 0 };
static Points points[5] = { 0 };

// NOTE: Defined triangle is isosceles with common angles of 70 degrees.
static float shipHeight = 0.0f;
static float gravity = 0.0f;

static int countmediumBallss = 0;
static int countsmallBallss = 0;
static int meteorsDestroyed = 0;
static rf_vec2 linePosition = { 0 };

static bool victory = false;
static bool lose = false;
static bool awake = false;

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
    .title  = "sample game: pang"
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
static void InitGame(void)
{
    int posx, posy;
    int velx = 0;
    int vely = 0;

    framesCounter = 0;
    gameOver = false;
    pause = false;
    score = 0;

    victory = false;
    lose = false;
    awake = true;
    gravity = 0.25f;

    linePosition = (rf_vec2){ 0.0f , 0.0f };
    shipHeight = (PLAYER_BASE_SIZE / 2) / tanf(20 * RF_DEG2RAD);

    // Initialization player
    player.position = (rf_vec2){ window.width / 2, window.height };
    player.speed = (rf_vec2){ PLAYER_SPEED, PLAYER_SPEED };
    player.rotation = 0;
    player.collider = (rf_vec3){ player.position.x, player.position.y - shipHeight / 2.0f, 12.0f };

    meteorsDestroyed = 0;

    // Initialize shoots
    for (int i = 0; i < PLAYER_MAX_SHOOTS; i++)
    {
        shoot[i].position = (rf_vec2){ 0, 0 };
        shoot[i].speed = (rf_vec2){ 0, 0 };
        shoot[i].radius = 2;
        shoot[i].active = false;
        shoot[i].lifeSpawn = 0;
    }

    // Initialize big meteors
    for (int i = 0; i < MAX_BIG_BALLS; i++)
    {
        bigBalls[i].radius = 40.0f;
        posx = GetRandomValue(0 + bigBalls[i].radius, window.width - bigBalls[i].radius);
        posy = GetRandomValue(0 + bigBalls[i].radius, window.height / 2);

        bigBalls[i].position = (rf_vec2){ posx, posy };

        while ((velx == 0) || (vely == 0))
        {
            velx = GetRandomValue(-BALLS_SPEED, BALLS_SPEED);
            vely = GetRandomValue(-BALLS_SPEED, BALLS_SPEED);
        }

        bigBalls[i].speed = (rf_vec2){ velx, vely };
        bigBalls[i].points = 200;
        bigBalls[i].active = true;
    }

    // Initialize medium meteors
    for (int i = 0; i < MAX_BIG_BALLS * 2; i++)
    {
        mediumBalls[i].position = (rf_vec2){ -100, -100 };
        mediumBalls[i].speed = (rf_vec2){ 0,0 };
        mediumBalls[i].radius = 20.0f;
        mediumBalls[i].points = 100;
        mediumBalls[i].active = false;
    }

    // Initialize small meteors
    for (int i = 0; i < MAX_BIG_BALLS * 4; i++)
    {
        smallBalls[i].position = (rf_vec2){ -100, -100 };
        smallBalls[i].speed = (rf_vec2){ 0, 0 };
        smallBalls[i].radius = 10.0f;
        smallBalls[i].points = 50;
        smallBalls[i].active = false;
    }

    // Initialize animated points
    for (int i = 0; i < 5; i++)
    {
        points[i].position = (rf_vec2){ 0, 0 };
        points[i].value = 0;
        points[i].alpha = 0.0f;
    }

    countmediumBallss = 0;
    countsmallBallss = 0;
}

// Update game (one frame)
void UpdateGame(const platform_input_state* input)
{
    if (!gameOver && !victory)
    {
        if (input->keys[KEYCODE_P] == KEY_PRESSED_DOWN) pause = !pause;

        if (!pause)
        {
            // Player logic
            if (input->keys[KEYCODE_LEFT] == KEY_HOLD_DOWN)  player.position.x -= player.speed.x;
            if (input->keys[KEYCODE_RIGHT] == KEY_HOLD_DOWN)  player.position.x += player.speed.x;

            // Player vs wall collision logic
            if (player.position.x + PLAYER_BASE_SIZE / 2 > window.width) player.position.x = window.width - PLAYER_BASE_SIZE / 2;
            else if (player.position.x - PLAYER_BASE_SIZE / 2 < 0) player.position.x = 0 + PLAYER_BASE_SIZE / 2;

            // Player shot logic
            if (input->keys[KEYCODE_SPACE] == KEY_PRESSED_DOWN)
            {
                for (int i = 0; i < PLAYER_MAX_SHOOTS; i++)
                {
                    if (!shoot[i].active)
                    {
                        shoot[i].position = (rf_vec2){ player.position.x, player.position.y - shipHeight };
                        shoot[i].speed.y = PLAYER_SPEED;
                        shoot[i].active = true;

                        linePosition = (rf_vec2){ player.position.x, player.position.y };

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
                    shoot[i].position.y -= shoot[i].speed.y;

                    // Shot vs walls collision logic
                    if ((shoot[i].position.x > window.width + shoot[i].radius) || (shoot[i].position.x < 0 - shoot[i].radius) ||
                        (shoot[i].position.y > window.height + shoot[i].radius) || (shoot[i].position.y < 0 - shoot[i].radius))
                    {
                        shoot[i].active = false;
                        shoot[i].lifeSpawn = 0;
                    }

                    // Player shot life spawn
                    if (shoot[i].lifeSpawn >= 120)
                    {
                        shoot[i].position = (rf_vec2){ 0.0f, 0.0f };
                        shoot[i].speed = (rf_vec2){ 0.0f, 0.0f };
                        shoot[i].lifeSpawn = 0;
                        shoot[i].active = false;
                    }
                }
            }

            // Player vs meteors collision logic
            player.collider = (rf_vec3){ player.position.x, player.position.y - shipHeight / 2, 12 };

            for (int i = 0; i < MAX_BIG_BALLS; i++)
            {
                if (rf_check_collision_circles((rf_vec2) { player.collider.x, player.collider.y }, player.collider.z, bigBalls[i].position, bigBalls[i].radius) && bigBalls[i].active)
                {
                    gameOver = true;
                }
            }

            for (int i = 0; i < MAX_BIG_BALLS * 2; i++)
            {
                if (rf_check_collision_circles((rf_vec2) { player.collider.x, player.collider.y }, player.collider.z, mediumBalls[i].position, mediumBalls[i].radius) && mediumBalls[i].active)
                {
                    gameOver = true;
                }
            }

            for (int i = 0; i < MAX_BIG_BALLS * 4; i++)
            {
                if (rf_check_collision_circles((rf_vec2) { player.collider.x, player.collider.y }, player.collider.z, smallBalls[i].position, smallBalls[i].radius) && smallBalls[i].active)
                {
                    gameOver = true;
                }
            }

            // Meteors logic (big)
            for (int i = 0; i < MAX_BIG_BALLS; i++)
            {
                if (bigBalls[i].active)
                {
                    // Meteor movement logic
                    bigBalls[i].position.x += bigBalls[i].speed.x;
                    bigBalls[i].position.y += bigBalls[i].speed.y;

                    // Meteor vs wall collision logic
                    if (((bigBalls[i].position.x + bigBalls[i].radius) >= window.width) || ((bigBalls[i].position.x - bigBalls[i].radius) <= 0)) bigBalls[i].speed.x *= -1;
                    if ((bigBalls[i].position.y - bigBalls[i].radius) <= 0) bigBalls[i].speed.y *= -1.5;

                    if ((bigBalls[i].position.y + bigBalls[i].radius) >= window.height)
                    {
                        bigBalls[i].speed.y *= -1;
                        bigBalls[i].position.y = window.height - bigBalls[i].radius;
                    }

                    bigBalls[i].speed.y += gravity;
                }
            }

            // Meteors logic (medium)
            for (int i = 0; i < MAX_BIG_BALLS * 2; i++)
            {
                if (mediumBalls[i].active)
                {
                    // Meteor movement logic
                    mediumBalls[i].position.x += mediumBalls[i].speed.x;
                    mediumBalls[i].position.y += mediumBalls[i].speed.y;

                    // Meteor vs wall collision logic
                    if (mediumBalls[i].position.x + mediumBalls[i].radius >= window.width || mediumBalls[i].position.x - mediumBalls[i].radius <= 0) mediumBalls[i].speed.x *= -1;
                    if (mediumBalls[i].position.y - mediumBalls[i].radius <= 0) mediumBalls[i].speed.y *= -1;
                    if (mediumBalls[i].position.y + mediumBalls[i].radius >= window.height)
                    {
                        mediumBalls[i].speed.y *= -1;
                        mediumBalls[i].position.y = window.height - mediumBalls[i].radius;
                    }

                    mediumBalls[i].speed.y += gravity + 0.12f;
                }
            }

            // Meteors logic (small)
            for (int i = 0; i < MAX_BIG_BALLS * 4; i++)
            {
                if (smallBalls[i].active)
                {
                    // Meteor movement logic
                    smallBalls[i].position.x += smallBalls[i].speed.x;
                    smallBalls[i].position.y += smallBalls[i].speed.y;

                    // Meteor vs wall collision logic
                    if (smallBalls[i].position.x + smallBalls[i].radius >= window.width || smallBalls[i].position.x - smallBalls[i].radius <= 0) smallBalls[i].speed.x *= -1;
                    if (smallBalls[i].position.y - smallBalls[i].radius <= 0) smallBalls[i].speed.y *= -1;
                    if (smallBalls[i].position.y + smallBalls[i].radius >= window.height)
                    {
                        smallBalls[i].speed.y *= -1;
                        smallBalls[i].position.y = window.height - smallBalls[i].radius;
                    }

                    smallBalls[i].speed.y += gravity + 0.25f;
                }
            }

            // Player-shot vs meteors logic
            for (int i = 0; i < PLAYER_MAX_SHOOTS; i++)
            {
                if ((shoot[i].active))
                {
                    for (int a = 0; a < MAX_BIG_BALLS; a++)
                    {
                        if (bigBalls[a].active && (bigBalls[a].position.x - bigBalls[a].radius <= linePosition.x && bigBalls[a].position.x + bigBalls[a].radius >= linePosition.x)
                            && (bigBalls[a].position.y + bigBalls[a].radius >= shoot[i].position.y))
                        {
                            shoot[i].active = false;
                            shoot[i].lifeSpawn = 0;
                            bigBalls[a].active = false;
                            meteorsDestroyed++;
                            score += bigBalls[a].points;

                            for (int z = 0; z < 5; z++)
                            {
                                if (points[z].alpha == 0.0f)
                                {
                                    points[z].position = bigBalls[a].position;
                                    points[z].value = bigBalls[a].points;
                                    points[z].alpha = 1.0f;
                                    z = 5;
                                }
                            }

                            for (int j = 0; j < 2; j++)
                            {
                                if ((countmediumBallss % 2) == 0)
                                {
                                    mediumBalls[countmediumBallss].position = (rf_vec2){ bigBalls[a].position.x, bigBalls[a].position.y };
                                    mediumBalls[countmediumBallss].speed = (rf_vec2){ -1 * BALLS_SPEED, BALLS_SPEED };
                                }
                                else
                                {
                                    mediumBalls[countmediumBallss].position = (rf_vec2){ bigBalls[a].position.x, bigBalls[a].position.y };
                                    mediumBalls[countmediumBallss].speed = (rf_vec2){ BALLS_SPEED, BALLS_SPEED };
                                }

                                mediumBalls[countmediumBallss].active = true;
                                countmediumBallss++;
                            }

                            a = MAX_BIG_BALLS;
                        }
                    }
                }

                if ((shoot[i].active))
                {
                    for (int b = 0; b < MAX_BIG_BALLS * 2; b++)
                    {
                        if (mediumBalls[b].active && (mediumBalls[b].position.x - mediumBalls[b].radius <= linePosition.x && mediumBalls[b].position.x + mediumBalls[b].radius >= linePosition.x)
                            && (mediumBalls[b].position.y + mediumBalls[b].radius >= shoot[i].position.y))
                        {
                            shoot[i].active = false;
                            shoot[i].lifeSpawn = 0;
                            mediumBalls[b].active = false;
                            meteorsDestroyed++;
                            score += mediumBalls[b].points;

                            for (int z = 0; z < 5; z++)
                            {
                                if (points[z].alpha == 0.0f)
                                {
                                    points[z].position = mediumBalls[b].position;
                                    points[z].value = mediumBalls[b].points;
                                    points[z].alpha = 1.0f;
                                    z = 5;
                                }
                            }

                            for (int j = 0; j < 2; j++)
                            {
                                if (countsmallBallss % 2 == 0)
                                {
                                    smallBalls[countsmallBallss].position = (rf_vec2){ mediumBalls[b].position.x, mediumBalls[b].position.y };
                                    smallBalls[countsmallBallss].speed = (rf_vec2){ BALLS_SPEED * -1, BALLS_SPEED * -1 };
                                }
                                else
                                {
                                    smallBalls[countsmallBallss].position = (rf_vec2){ mediumBalls[b].position.x, mediumBalls[b].position.y };
                                    smallBalls[countsmallBallss].speed = (rf_vec2){ BALLS_SPEED, BALLS_SPEED * -1 };
                                }

                                smallBalls[countsmallBallss].active = true;
                                countsmallBallss++;
                            }

                            b = MAX_BIG_BALLS * 2;
                        }
                    }
                }

                if ((shoot[i].active))
                {
                    for (int c = 0; c < MAX_BIG_BALLS * 4; c++)
                    {
                        if (smallBalls[c].active && (smallBalls[c].position.x - smallBalls[c].radius <= linePosition.x && smallBalls[c].position.x + smallBalls[c].radius >= linePosition.x)
                            && (smallBalls[c].position.y + smallBalls[c].radius >= shoot[i].position.y))
                        {
                            shoot[i].active = false;
                            shoot[i].lifeSpawn = 0;
                            smallBalls[c].active = false;
                            meteorsDestroyed++;
                            score += smallBalls[c].points;

                            for (int z = 0; z < 5; z++)
                            {
                                if (points[z].alpha == 0.0f)
                                {
                                    points[z].position = smallBalls[c].position;
                                    points[z].value = smallBalls[c].points;
                                    points[z].alpha = 1.0f;
                                    z = 5;
                                }
                            }

                            c = MAX_BIG_BALLS * 4;
                        }
                    }
                }
            }

            if (meteorsDestroyed == (MAX_BIG_BALLS + MAX_BIG_BALLS * 2 + MAX_BIG_BALLS * 4)) victory = true;
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

    // Points move-up and fade logic
    for (int z = 0; z < 5; z++)
    {
        if (points[z].alpha > 0.0f)
        {
            points[z].position.y -= 2;
            points[z].alpha -= 0.02f;
        }

        if (points[z].alpha < 0.0f) points[z].alpha = 0.0f;
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
            // Draw player
            rf_vec2 v1 = { player.position.x + sinf(player.rotation * RF_DEG2RAD) * (shipHeight), player.position.y - cosf(player.rotation * RF_DEG2RAD) * (shipHeight) };
            rf_vec2 v2 = { player.position.x - cosf(player.rotation * RF_DEG2RAD) * (PLAYER_BASE_SIZE / 2), player.position.y - sinf(player.rotation * RF_DEG2RAD) * (PLAYER_BASE_SIZE / 2) };
            rf_vec2 v3 = { player.position.x + cosf(player.rotation * RF_DEG2RAD) * (PLAYER_BASE_SIZE / 2), player.position.y + sinf(player.rotation * RF_DEG2RAD) * (PLAYER_BASE_SIZE / 2) };
            rf_draw_triangle(v1, v2, v3, RF_MAROON);

            // Draw meteors (big)
            for (int i = 0; i < MAX_BIG_BALLS; i++)
            {
                if (bigBalls[i].active) rf_draw_circle_v(bigBalls[i].position, bigBalls[i].radius, RF_DARKGRAY);
                else rf_draw_circle_v(bigBalls[i].position, bigBalls[i].radius, rf_fade(RF_LIGHTGRAY, 0.3f));
            }

            // Draw meteors (medium)
            for (int i = 0; i < MAX_BIG_BALLS * 2; i++)
            {
                if (mediumBalls[i].active) rf_draw_circle_v(mediumBalls[i].position, mediumBalls[i].radius, RF_GRAY);
                else rf_draw_circle_v(mediumBalls[i].position, mediumBalls[i].radius, rf_fade(RF_LIGHTGRAY, 0.3f));
            }

            // Draw meteors (small)
            for (int i = 0; i < MAX_BIG_BALLS * 4; i++)
            {
                if (smallBalls[i].active) rf_draw_circle_v(smallBalls[i].position, smallBalls[i].radius, RF_GRAY);
                else rf_draw_circle_v(smallBalls[i].position, smallBalls[i].radius, rf_fade(RF_LIGHTGRAY, 0.3f));
            }

            // Draw shoot
            for (int i = 0; i < PLAYER_MAX_SHOOTS; i++)
            {
                if (shoot[i].active) rf_draw_line(linePosition.x, linePosition.y, shoot[i].position.x, shoot[i].position.y, RF_RED);
            }

            // Draw score points
            for (int z = 0; z < 5; z++)
            {
                if (points[z].alpha > 0.0f)
                {
                    rf_draw_text(TextFormat("+%02i", points[z].value), points[z].position.x, points[z].position.y, 20, rf_fade(RF_BLUE, points[z].alpha));
                }
            }

            // Draw score (UI)
            rf_draw_text(TextFormat("SCORE: %i", score), 10, 10, 20, RF_LIGHTGRAY);

            if (victory)
            {
                rf_draw_text("YOU WIN!", window.width / 2 - rf_measure_text(rf_get_default_font(), "YOU WIN!", 60, 0).width / 2, 100, 60, RF_LIGHTGRAY);
                rf_draw_text("PRESS [ENTER] TO PLAY AGAIN", window.width / 2 - rf_measure_text(rf_get_default_font(), "PRESS [ENTER] TO PLAY AGAIN", 20, 0).width / 2, window.height / 2 - 50, 20, RF_LIGHTGRAY);
            }
            
            if (pause) rf_draw_text("GAME PAUSED", window.width / 2 - rf_measure_text(rf_get_default_font(), "GAME PAUSED", 40, 0).width / 2, window.height / 2 - 40, 40, RF_LIGHTGRAY);
        }
        else rf_draw_text("PRESS [ENTER] TO PLAY AGAIN", window.width / 2 - rf_measure_text(rf_get_default_font(), "PRESS [ENTER] TO PLAY AGAIN", 20, 0).width / 2, window.height / 2 - 50, 20, RF_LIGHTGRAY);
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