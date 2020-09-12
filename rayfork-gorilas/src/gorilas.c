/*******************************************************************************************
*
*   raylib - sample game: gorilas
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
#define MAX_BUILDINGS                    15
#define MAX_EXPLOSIONS                  200
#define MAX_PLAYERS                       2

#define BUILDING_RELATIVE_ERROR          30        // Building size random range %
#define BUILDING_MIN_RELATIVE_HEIGHT     20        // Minimum height in % of the window.height
#define BUILDING_MAX_RELATIVE_HEIGHT     60        // Maximum height in % of the window.height
#define BUILDING_MIN_GRAYSCALE_COLOR    120        // Minimum gray color for the buildings
#define BUILDING_MAX_GRAYSCALE_COLOR    200        // Maximum gray color for the buildings

#define MIN_PLAYER_POSITION               5        // Minimum x position %
#define MAX_PLAYER_POSITION              20        // Maximum x position %

#define GRAVITY                       9.81f
#define DELTA_FPS                        60

//----------------------------------------------------------------------------------
// Types and Structures Definition
//----------------------------------------------------------------------------------
typedef struct Player {
    rf_vec2 position;
    rf_vec2 size;

    rf_vec2 aimingPoint;
    int aimingAngle;
    int aimingPower;

    rf_vec2 previousPoint;
    int previousAngle;
    int previousPower;

    rf_vec2 impactPoint;

    bool isLeftTeam;                // This player belongs to the left or to the right team
    bool isPlayer;                  // If is a player or an AI
    bool isAlive;
} Player;

typedef struct Building {
    rf_rec rectangle;
    rf_color color;
} Building;

typedef struct Explosion {
    rf_vec2 position;
    int radius;
    bool active;
} Explosion;

typedef struct Ball {
    rf_vec2 position;
    rf_vec2 speed;
    int radius;
    bool active;
} Ball;

//------------------------------------------------------------------------------------
// Global Variables Declaration
//------------------------------------------------------------------------------------
static bool gameOver = false;
static bool pause = false;

static Player player[MAX_PLAYERS] = { 0 };
static Building building[MAX_BUILDINGS] = { 0 };
static Explosion explosion[MAX_EXPLOSIONS] = { 0 };
static Ball ball = { 0 };

static int playerTurn = 0;
static bool ballOnAir = false;

//------------------------------------------------------------------------------------
// Module Functions Declaration (local)
//------------------------------------------------------------------------------------
static void InitGame(void);                                      // Initialize game
static void UpdateGame(const platform_input_state* input);       // Update game (one frame)
static void DrawGame(void);                                      // Draw game (one frame)
static void UnloadGame(void);                                    // Unload game
static void UpdateDrawFrame(const platform_input_state* input);  // Update and Draw (one frame)

// Additional module functions
static void InitBuildings(void);
static void InitPlayers(void);
static bool UpdatePlayer(int playerTurn, const platform_input_state* input);
static bool UpdateBall(int playerTurn);

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
    .title  = "sample game: gorilas"
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
    // Init shoot
    ball.radius = 10;
    ballOnAir = false;
    ball.active = false;

    InitBuildings();
    InitPlayers();

    // Init explosions
    for (int i = 0; i < MAX_EXPLOSIONS; i++)
    {
        explosion[i].position = (rf_vec2){ 0.0f, 0.0f };
        explosion[i].radius = 30;
        explosion[i].active = false;
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
            if (!ballOnAir) ballOnAir = UpdatePlayer(playerTurn, input); // If we are aiming
            else
            {
                if (UpdateBall(playerTurn))                       // If collision
                {
                    // Game over logic
                    bool leftTeamAlive = false;
                    bool rightTeamAlive = false;

                    for (int i = 0; i < MAX_PLAYERS; i++)
                    {
                        if (player[i].isAlive)
                        {
                            if (player[i].isLeftTeam) leftTeamAlive = true;
                            if (!player[i].isLeftTeam) rightTeamAlive = true;
                        }
                    }

                    if (leftTeamAlive && rightTeamAlive)
                    {
                        ballOnAir = false;
                        ball.active = false;

                        playerTurn++;

                        if (playerTurn == MAX_PLAYERS) playerTurn = 0;
                    }
                    else
                    {
                        gameOver = true;

                        // if (leftTeamAlive) left team wins
                        // if (rightTeamAlive) right team wins
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
            // Draw buildings
            for (int i = 0; i < MAX_BUILDINGS; i++) rf_draw_rectangle_rec(building[i].rectangle, building[i].color);

            // Draw explosions
            for (int i = 0; i < MAX_EXPLOSIONS; i++)
            {
                if (explosion[i].active) rf_draw_circle(explosion[i].position.x, explosion[i].position.y, explosion[i].radius, RF_RAYWHITE);
            }

            // Draw players
            for (int i = 0; i < MAX_PLAYERS; i++)
            {
                if (player[i].isAlive)
                {
                    if (player[i].isLeftTeam) rf_draw_rectangle(player[i].position.x - player[i].size.x / 2, player[i].position.y - player[i].size.y / 2,
                        player[i].size.x, player[i].size.y, RF_BLUE);
                    else rf_draw_rectangle(player[i].position.x - player[i].size.x / 2, player[i].position.y - player[i].size.y / 2,
                        player[i].size.x, player[i].size.y, RF_RED);
                }
            }

            // Draw ball
            if (ball.active) rf_draw_circle(ball.position.x, ball.position.y, ball.radius, RF_MAROON);

            // Draw the angle and the power of the aim, and the previous ones
            if (!ballOnAir)
            {
                // Draw shot information
                /*
                if (player[playerTurn].isLeftTeam)
                {
                    DrawText(TextFormat("Previous Point %i, %i", (int)player[playerTurn].previousPoint.x, (int)player[playerTurn].previousPoint.y), 20, 20, 20, DARKBLUE);
                    DrawText(TextFormat("Previous Angle %i", player[playerTurn].previousAngle), 20, 50, 20, DARKBLUE);
                    DrawText(TextFormat("Previous Power %i", player[playerTurn].previousPower), 20, 80, 20, DARKBLUE);
                    DrawText(TextFormat("Aiming Point %i, %i", (int)player[playerTurn].aimingPoint.x, (int)player[playerTurn].aimingPoint.y), 20, 110, 20, DARKBLUE);
                    DrawText(TextFormat("Aiming Angle %i", player[playerTurn].aimingAngle), 20, 140, 20, DARKBLUE);
                    DrawText(TextFormat("Aiming Power %i", player[playerTurn].aimingPower), 20, 170, 20, DARKBLUE);
                }
                else
                {
                    DrawText(TextFormat("Previous Point %i, %i", (int)player[playerTurn].previousPoint.x, (int)player[playerTurn].previousPoint.y), window.width*3/4, 20, 20, DARKBLUE);
                    DrawText(TextFormat("Previous Angle %i", player[playerTurn].previousAngle), window.width*3/4, 50, 20, DARKBLUE);
                    DrawText(TextFormat("Previous Power %i", player[playerTurn].previousPower), window.width*3/4, 80, 20, DARKBLUE);
                    DrawText(TextFormat("Aiming Point %i, %i", (int)player[playerTurn].aimingPoint.x, (int)player[playerTurn].aimingPoint.y), window.width*3/4, 110, 20, DARKBLUE);
                    DrawText(TextFormat("Aiming Angle %i", player[playerTurn].aimingAngle), window.width*3/4, 140, 20, DARKBLUE);
                    DrawText(TextFormat("Aiming Power %i", player[playerTurn].aimingPower), window.width*3/4, 170, 20, DARKBLUE);
                }
                */

                // Draw aim
                if (player[playerTurn].isLeftTeam)
                {
                    // Previous aiming
                    rf_draw_triangle((rf_vec2) { player[playerTurn].position.x - player[playerTurn].size.x / 4, player[playerTurn].position.y - player[playerTurn].size.y / 4 },
                        (rf_vec2) {
                        player[playerTurn].position.x + player[playerTurn].size.x / 4, player[playerTurn].position.y + player[playerTurn].size.y / 4
                    },
                        player[playerTurn].previousPoint, RF_GRAY);

                    // Actual aiming
                    rf_draw_triangle((rf_vec2) { player[playerTurn].position.x - player[playerTurn].size.x / 4, player[playerTurn].position.y - player[playerTurn].size.y / 4 },
                        (rf_vec2) {
                        player[playerTurn].position.x + player[playerTurn].size.x / 4, player[playerTurn].position.y + player[playerTurn].size.y / 4
                    },
                        player[playerTurn].aimingPoint, RF_DARKBLUE);
                }
                else
                {
                    // Previous aiming
                    rf_draw_triangle((rf_vec2) { player[playerTurn].position.x - player[playerTurn].size.x / 4, player[playerTurn].position.y + player[playerTurn].size.y / 4 },
                        (rf_vec2) {
                        player[playerTurn].position.x + player[playerTurn].size.x / 4, player[playerTurn].position.y - player[playerTurn].size.y / 4
                    },
                        player[playerTurn].previousPoint, RF_GRAY);

                    // Actual aiming
                    rf_draw_triangle((rf_vec2) { player[playerTurn].position.x - player[playerTurn].size.x / 4, player[playerTurn].position.y + player[playerTurn].size.y / 4 },
                        (rf_vec2) {
                        player[playerTurn].position.x + player[playerTurn].size.x / 4, player[playerTurn].position.y - player[playerTurn].size.y / 4
                    },
                        player[playerTurn].aimingPoint, RF_MAROON);
                }
            }

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
static void InitBuildings(void)
{
    // Horizontal generation
    int currentWidth = 0;

    // We make sure the absolute error randomly generated for each building, has as a minimum value the window.width.
    // This way all the screen will be filled with buildings. Each building will have a different, random width.

    float relativeWidth = 100 / (100 - BUILDING_RELATIVE_ERROR);
    float buildingWidthMean = (window.width * relativeWidth / MAX_BUILDINGS) + 1;        // We add one to make sure we will cover the whole screen.

    // Vertical generation
    int currentHeighth = 0;
    int grayLevel;

    // Creation
    for (int i = 0; i < MAX_BUILDINGS; i++)
    {
        // Horizontal
        building[i].rectangle.x = currentWidth;
        building[i].rectangle.width = GetRandomValue(buildingWidthMean * (100 - BUILDING_RELATIVE_ERROR / 2) / 100 + 1, buildingWidthMean * (100 + BUILDING_RELATIVE_ERROR) / 100);

        currentWidth += building[i].rectangle.width;

        // Vertical
        currentHeighth = GetRandomValue(BUILDING_MIN_RELATIVE_HEIGHT, BUILDING_MAX_RELATIVE_HEIGHT);
        building[i].rectangle.y = window.height - (window.height * currentHeighth / 100);
        building[i].rectangle.height = window.height * currentHeighth / 100 + 1;

        // Color
        grayLevel = GetRandomValue(BUILDING_MIN_GRAYSCALE_COLOR, BUILDING_MAX_GRAYSCALE_COLOR);
        building[i].color = (rf_color){ grayLevel, grayLevel, grayLevel, 255 };
    }
}

static void InitPlayers(void)
{
    for (int i = 0; i < MAX_PLAYERS; i++)
    {
        player[i].isAlive = true;

        // Decide the team of this player
        if (i % 2 == 0) player[i].isLeftTeam = true;
        else player[i].isLeftTeam = false;

        // Now there is no AI
        player[i].isPlayer = true;

        // Set size, by default by now
        player[i].size = (rf_vec2){ 40, 40 };

        // Set position
        if (player[i].isLeftTeam) player[i].position.x = GetRandomValue(window.width * MIN_PLAYER_POSITION / 100, window.width * MAX_PLAYER_POSITION / 100);
        else player[i].position.x = window.width - GetRandomValue(window.width * MIN_PLAYER_POSITION / 100, window.width * MAX_PLAYER_POSITION / 100);

        for (int j = 0; j < MAX_BUILDINGS; j++)
        {
            if (building[j].rectangle.x > player[i].position.x)
            {
                // Set the player in the center of the building
                player[i].position.x = building[j - 1].rectangle.x + building[j - 1].rectangle.width / 2;
                // Set the player at the top of the building
                player[i].position.y = building[j - 1].rectangle.y - player[i].size.y / 2;
                break;
            }
        }

        // Set statistics to 0
        player[i].aimingPoint = player[i].position;
        player[i].previousAngle = 0;
        player[i].previousPower = 0;
        player[i].previousPoint = player[i].position;
        player[i].aimingAngle = 0;
        player[i].aimingPower = 0;

        player[i].impactPoint = (rf_vec2){ -100, -100 };
    }
}

static bool UpdatePlayer(int playerTurn, const platform_input_state* input)
{
    // If we are aiming at the firing quadrant, we calculate the angle
    if (input->mouse_y <= player[playerTurn].position.y)
    {
        // Left team
        if (player[playerTurn].isLeftTeam && input->mouse_x >= player[playerTurn].position.x)
        {
            // Distance (calculating the fire power)
            player[playerTurn].aimingPower = sqrt(pow(player[playerTurn].position.x - input->mouse_x, 2) + pow(player[playerTurn].position.y - input->mouse_y, 2));
            // Calculates the angle via arcsin
            player[playerTurn].aimingAngle = asin((player[playerTurn].position.y - input->mouse_y) / player[playerTurn].aimingPower) * RF_RAD2DEG;
            // Point of the screen we are aiming at
            player[playerTurn].aimingPoint = (rf_vec2) { input->mouse_x, input->mouse_y };

            // Ball fired
            if (input->left_mouse_btn == BTN_PRESSED_DOWN)
            {
                player[playerTurn].previousPoint = player[playerTurn].aimingPoint;
                player[playerTurn].previousPower = player[playerTurn].aimingPower;
                player[playerTurn].previousAngle = player[playerTurn].aimingAngle;
                ball.position = player[playerTurn].position;

                return true;
            }
        }
        // Right team
        else if (!player[playerTurn].isLeftTeam && input->mouse_x <= player[playerTurn].position.x)
        {
            // Distance (calculating the fire power)
            player[playerTurn].aimingPower = sqrt(pow(player[playerTurn].position.x - input->mouse_x, 2) + pow(player[playerTurn].position.y - input->mouse_y, 2));
            // Calculates the angle via arcsin
            player[playerTurn].aimingAngle = asin((player[playerTurn].position.y - input->mouse_y) / player[playerTurn].aimingPower) * RF_RAD2DEG;
            // Point of the screen we are aiming at
            player[playerTurn].aimingPoint = (rf_vec2){ input->mouse_x, input->mouse_y };

            // Ball fired
            if (input->left_mouse_btn == BTN_PRESSED_DOWN)
            {
                player[playerTurn].previousPoint = player[playerTurn].aimingPoint;
                player[playerTurn].previousPower = player[playerTurn].aimingPower;
                player[playerTurn].previousAngle = player[playerTurn].aimingAngle;
                ball.position = player[playerTurn].position;

                return true;
            }
        }
        else
        {
            player[playerTurn].aimingPoint = player[playerTurn].position;
            player[playerTurn].aimingPower = 0;
            player[playerTurn].aimingAngle = 0;
        }
    }
    else
    {
        player[playerTurn].aimingPoint = player[playerTurn].position;
        player[playerTurn].aimingPower = 0;
        player[playerTurn].aimingAngle = 0;
    }

    return false;
}

static bool UpdateBall(int playerTurn)
{
    static int explosionNumber = 0;

    // Activate ball
    if (!ball.active)
    {
        if (player[playerTurn].isLeftTeam)
        {
            ball.speed.x = cos(player[playerTurn].previousAngle * RF_DEG2RAD) * player[playerTurn].previousPower * 3 / DELTA_FPS;
            ball.speed.y = -sin(player[playerTurn].previousAngle * RF_DEG2RAD) * player[playerTurn].previousPower * 3 / DELTA_FPS;
            ball.active = true;
        }
        else
        {
            ball.speed.x = -cos(player[playerTurn].previousAngle * RF_DEG2RAD) * player[playerTurn].previousPower * 3 / DELTA_FPS;
            ball.speed.y = -sin(player[playerTurn].previousAngle * RF_DEG2RAD) * player[playerTurn].previousPower * 3 / DELTA_FPS;
            ball.active = true;
        }
    }

    ball.position.x += ball.speed.x;
    ball.position.y += ball.speed.y;
    ball.speed.y += GRAVITY / DELTA_FPS;

    // Collision
    if (ball.position.x + ball.radius < 0) return true;
    else if (ball.position.x - ball.radius > window.width) return true;
    else
    {
        // Player collision
        for (int i = 0; i < MAX_PLAYERS; i++)
        {
            if (rf_check_collision_circle_rec(ball.position, ball.radius, (rf_rec) {
                player[i].position.x - player[i].size.x / 2, player[i].position.y - player[i].size.y / 2,
                    player[i].size.x, player[i].size.y
            }))
            {
                // We can't hit ourselves
                if (i == playerTurn) return false;
                else
                {
                    // We set the impact point
                    player[playerTurn].impactPoint.x = ball.position.x;
                    player[playerTurn].impactPoint.y = ball.position.y + ball.radius;

                    // We destroy the player
                    player[i].isAlive = false;
                    return true;
                }
            }
        }

        // Building collision
        // NOTE: We only check building collision if we are not inside an explosion
        for (int i = 0; i < MAX_BUILDINGS; i++)
        {
            if (rf_check_collision_circles(ball.position, ball.radius, explosion[i].position, explosion[i].radius - ball.radius))
            {
                return false;
            }
        }

        for (int i = 0; i < MAX_BUILDINGS; i++)
        {
            if (rf_check_collision_circle_rec(ball.position, ball.radius, building[i].rectangle))
            {
                // We set the impact point
                player[playerTurn].impactPoint.x = ball.position.x;
                player[playerTurn].impactPoint.y = ball.position.y + ball.radius;

                // We create an explosion
                explosion[explosionNumber].position = player[playerTurn].impactPoint;
                explosion[explosionNumber].active = true;
                explosionNumber++;

                return true;
            }
        }
    }

    return false;
}