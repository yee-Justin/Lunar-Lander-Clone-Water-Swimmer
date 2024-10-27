/**
* Author: Justin Yee
* Assignment: Lunar Lander
* Date due: 2024-10-27, 11:59pm
* I pledge that I have completed this assignment without
* collaborating with anyone else, in conformance with the
* NYU School of Engineering Policies and Procedures on
* Academic Misconduct.
**/



#define GL_SILENCE_DEPRECATION
#define STB_IMAGE_IMPLEMENTATION
#define LOG(argument) std::cout << argument << '\n'
#define GL_GLEXT_PROTOTYPES 1
#define FIXED_TIMESTEP 0.0166666f
#define PLATFORM_COUNT 13
#define CEILING_COUNT 13
#define LEFTWALL_COUNT 10
#define RIGHTWALL_COUNT 10
#define FOOD_COUNT 7

#ifdef _WINDOWS
#include <GL/glew.h>
#endif

#include <SDL.h>
#include <SDL_opengl.h>
#include <SDL_mixer.h>
#include "glm/mat4x4.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "ShaderProgram.h"
#include "stb_image.h"
#include "cmath"
#include <ctime>
#include <vector>
#include <cstdlib>
#include "Entity.h"

// ––––– STRUCTS AND ENUMS ––––– //
struct GameState
{
    Entity* player;
    Entity* platforms;
    Entity* right_walls;
    Entity* left_walls;
    Entity* ceilings;
	Entity* background;
    Entity* midground;
    Entity* food;
    Entity* win;
    Entity* lose;
    Entity* air;
    Entity* fire;
    Entity* escape;
};

// ––––– CONSTANTS ––––– //
constexpr int WINDOW_WIDTH  = 1536,
          WINDOW_HEIGHT = 384;

constexpr float BG_RED     = 0.1922f,
            BG_BLUE    = 0.549f,
            BG_GREEN   = 0.9059f,
            BG_OPACITY = 1.0f;

constexpr int VIEWPORT_X = 0,
          VIEWPORT_Y = 0,
          VIEWPORT_WIDTH  = WINDOW_WIDTH,
          VIEWPORT_HEIGHT = WINDOW_HEIGHT;

constexpr char V_SHADER_PATH[] = "shaders/vertex_textured.glsl",
           F_SHADER_PATH[] = "shaders/fragment_textured.glsl";

constexpr float MILLISECONDS_IN_SECOND = 1000.0;
constexpr char SPRITESHEET_FILEPATH[] = "assets/whale.png"; //from https://rapidpunches.itch.io/blue-whale
constexpr char PLATFORM_FILEPATH[]    = "assets/floor.png"; //from https://ansimuz.itch.io/gothicvania-patreon-collection
constexpr char CEILING_FILEPATH[] = "assets/ceiling.png";//from https://ansimuz.itch.io/gothicvania-patreon-collection
constexpr char LEFTWALL_FILEPATH[] = "assets/leftwall.png";//from https://ansimuz.itch.io/gothicvania-patreon-collection
constexpr char RIGHTWALL_FILEPATH[] = "assets/rightwall.png";//from https://ansimuz.itch.io/gothicvania-patreon-collection
constexpr char BACKGROUND_FILEPATH[] = "assets/background.png";//from https://ansimuz.itch.io/gothicvania-patreon-collection
constexpr char MIDGROUND_FILEPATH[] = "assets/midground.png";//from https://ansimuz.itch.io/gothicvania-patreon-collection
constexpr char FOOD_FILEPATH[] = "assets/krill.png";//from https://rapidpunches.itch.io/blue-whale
constexpr char AIR_FILEPATH[] = "assets/air.png"; // from https://adwitr.itch.io/pixel-health-bar-asset-pack-2 (edited for color)
constexpr char WIN_FILEPATH[] = "assets/win.png";
constexpr char LOSE_FILEPATH[] = "assets/lose.png";
constexpr char FIRE_FILEPATH[] = "assets/fire.png"; //from https://govfx.itch.io/fire-vfx-free-pack
constexpr char ESCAPE_FILEPATH[] = "assets/escape.png"; //from https://pngtree.com/freepng/blue-vortex-black-hole_4491996.html

constexpr int NUMBER_OF_TEXTURES = 1;
constexpr GLint LEVEL_OF_DETAIL  = 0;
constexpr GLint TEXTURE_BORDER   = 0;

// ––––– GLOBAL VARIABLES ––––– //
GameState g_state;

SDL_Window* g_display_window;
bool g_game_is_running = true;

ShaderProgram g_program;
glm::mat4 g_view_matrix, g_projection_matrix;

float g_previous_ticks = 0.0f;
float g_accumulator = 0.0f;

int FOOD_EATEN = 0;

bool BELLY_FILLED = false;
bool WIN = false;
bool LOSE = false;

// ––––– GENERAL FUNCTIONS ––––– //
GLuint load_texture(const char* filepath)
{
    int width, height, number_of_components;
    unsigned char* image = stbi_load(filepath, &width, &height, &number_of_components, STBI_rgb_alpha);

    if (image == NULL)
    {
        LOG("Unable to load image. Make sure the path is correct.");
        assert(false);
    }

    GLuint textureID;
    glGenTextures(NUMBER_OF_TEXTURES, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexImage2D(GL_TEXTURE_2D, LEVEL_OF_DETAIL, GL_RGBA, width, height, TEXTURE_BORDER, GL_RGBA, GL_UNSIGNED_BYTE, image);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);


    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    stbi_image_free(image);

    return textureID;
}

void initialise()
{
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);
    g_display_window = SDL_CreateWindow("Water Swimmer",
                                      SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                      WINDOW_WIDTH, WINDOW_HEIGHT,
                                      SDL_WINDOW_OPENGL);

    SDL_GLContext context = SDL_GL_CreateContext(g_display_window);
    SDL_GL_MakeCurrent(g_display_window, context);

#ifdef _WINDOWS
    glewInit();
#endif

    // ––––– VIDEO ––––– //
    glViewport(VIEWPORT_X, VIEWPORT_Y, VIEWPORT_WIDTH, VIEWPORT_HEIGHT);

    g_program.load(V_SHADER_PATH, F_SHADER_PATH);

    g_view_matrix = glm::mat4(1.0f);
    g_projection_matrix = glm::ortho(-5.0f, 5.0f, -3.75f, 3.75f, -1.0f, 1.0f);

    g_program.set_projection_matrix(g_projection_matrix);
    g_program.set_view_matrix(g_view_matrix);

    glUseProgram(g_program.get_program_id());

    glClearColor(BG_RED, BG_BLUE, BG_GREEN, BG_OPACITY);


    // ––––– PLATFORMS ––––– //
    GLuint platform_texture_id = load_texture(PLATFORM_FILEPATH);
	GLuint background_texture_id = load_texture(BACKGROUND_FILEPATH);
	GLuint midground_texture_id = load_texture(MIDGROUND_FILEPATH);
	GLuint ceiling_texture_id = load_texture(CEILING_FILEPATH);
	GLuint leftwall_texture_id = load_texture(LEFTWALL_FILEPATH);
	GLuint rightwall_texture_id = load_texture(RIGHTWALL_FILEPATH);
	GLuint food_texture_id = load_texture(FOOD_FILEPATH);
	GLuint win_texture_id = load_texture(WIN_FILEPATH);
	GLuint lose_texture_id = load_texture(LOSE_FILEPATH);
	GLuint fire_texture_id = load_texture(FIRE_FILEPATH);
	GLuint escape_texture_id = load_texture(ESCAPE_FILEPATH);

    // Seed the random number generator
    srand(static_cast<unsigned int>(time(0)));
    g_state.food = new Entity[FOOD_COUNT];
    for (size_t i = 0; i < FOOD_COUNT; i++)
    {
        float x = ((rand() % 10) - 5)/2; // Generates -2.5-2.5
        float y = ((rand() % 10) - 5)/2; // Generates -2.5-2.5
        g_state.food[i].set_texture_id(food_texture_id);
        g_state.food[i].set_position(glm::vec3(x, y, 0.0f));
        g_state.food[i].set_scale(glm::vec3(0.15f, 0.15f, 1.0f));
        g_state.food[i].set_entity_type(FOOD);
        g_state.food[i].set_width(.25f);
        g_state.food[i].set_height(.25f);
        g_state.food[i].update(0.0f, NULL, NULL, 0);
    }

    g_state.platforms = new Entity[PLATFORM_COUNT];
    // Set the type of every platform entity to PLATFORM
    for (int i = 0; i < PLATFORM_COUNT; i++)
    {
        g_state.platforms[i].set_texture_id(platform_texture_id);
        g_state.platforms[i].set_position(glm::vec3(i - PLATFORM_COUNT / 2.0f, -3.5f, 0.0f));
        g_state.platforms[i].set_width(1.0f);
        g_state.platforms[i].set_height(.80f);
        g_state.platforms[i].set_entity_type(PLATFORM);
        g_state.platforms[i].update(0.0f, NULL, NULL, 0);
    }
    g_state.ceilings = new Entity[CEILING_COUNT];
    for (int i = 0; i < CEILING_COUNT; i++)
    {
        g_state.ceilings[i].set_texture_id(ceiling_texture_id);
        g_state.ceilings[i].set_position(glm::vec3(i - CEILING_COUNT / 1.8f, 3.5f, 0.0f));
        g_state.ceilings[i].set_width(1.0f);
        g_state.ceilings[i].set_height(.50f);
        g_state.ceilings[i].set_entity_type(PLATFORM);
        g_state.ceilings[i].update(0.0f, NULL, NULL, 0);
    }

    
    g_state.right_walls = new Entity[RIGHTWALL_COUNT];
    for (int i = 0; i < RIGHTWALL_COUNT; i++)
    {
        g_state.right_walls[i].set_texture_id(rightwall_texture_id);
        g_state.right_walls[i].set_position(glm::vec3(5.0f, i - RIGHTWALL_COUNT / 2.0f, 0.0f));
        g_state.right_walls[i].set_width(.1f);
        g_state.right_walls[i].set_height(1.0f);
        g_state.right_walls[i].set_scale(glm::vec3(.5f, 1.5f, 1.0f));
        g_state.right_walls[i].set_entity_type(WALL);
        g_state.right_walls[i].update(0.0f, NULL, NULL, 0);
    }
    g_state.left_walls = new Entity[LEFTWALL_COUNT];
    for (int i = 0; i < LEFTWALL_COUNT; i++)
    {
        g_state.left_walls[i].set_texture_id(leftwall_texture_id);
        g_state.left_walls[i].set_position(glm::vec3(-5.0f, i - LEFTWALL_COUNT / 2.0f, 0.0f));
        g_state.left_walls[i].set_width(.1f);
        g_state.left_walls[i].set_height(1.0f);
        g_state.left_walls[i].set_scale(glm::vec3(.5f, 1.5f, 1.0f));
        g_state.left_walls[i].set_entity_type(WALL);
        g_state.left_walls[i].update(0.0f, NULL, NULL, 0);
    }

    g_state.background = new Entity;
    g_state.background->set_texture_id(background_texture_id);
    g_state.background->set_position(glm::vec3(0.0f, 0.0f, 0.0f));
    g_state.background->set_scale(glm::vec3(10.0f, 10.0f, 1.0f));
    g_state.background->set_entity_type(BACKGROUND);
    g_state.background->update(0.0f, NULL, NULL, 0);

    g_state.midground = new Entity;
	g_state.midground->set_texture_id(midground_texture_id);
	g_state.midground->set_position(glm::vec3(0.0f, 0.0f, 0.0f));
	g_state.midground->set_scale(glm::vec3(10.0*1.6f, .75*10.0f, 1.0f));
	g_state.midground->set_entity_type(BACKGROUND);
	g_state.midground->update(0.0f, NULL, NULL, 0);

	g_state.air = new Entity;
	g_state.air->set_texture_id(load_texture(AIR_FILEPATH));
	g_state.air->set_position(glm::vec3(-4.4f, 3.3f, 0.0f));
	g_state.air->set_scale(glm::vec3(1.0f, .2f, 1.0f));
	g_state.air->set_entity_type(HEALTH);
	g_state.air->update(0.0f, NULL, NULL, 0);


    g_state.win = new Entity;
    g_state.win->set_texture_id(win_texture_id);
    g_state.win->set_position(glm::vec3(0.0f, 0.0f, 0.0f));
    g_state.win->set_scale(glm::vec3(2.0f, 2.0f, 1.0f));
    g_state.win->set_entity_type(BACKGROUND);
    g_state.win->update(0.0f, NULL, NULL, 0);

    g_state.lose = new Entity;
    g_state.lose->set_texture_id(lose_texture_id);
    g_state.lose->set_position(glm::vec3(0.0f, 0.0f, 0.0f));
    g_state.lose->set_scale(glm::vec3(2.0f, 2.0f, 1.0f));
    g_state.lose->set_entity_type(BACKGROUND);
    g_state.lose->update(0.0f, NULL, NULL, 0);

	g_state.escape = new Entity;
	g_state.escape->set_texture_id(escape_texture_id);
	g_state.escape->set_position(glm::vec3(0.0f, 0.0f, 0.0f));
	g_state.escape->set_scale(glm::vec3(2.0f, 2.0f, 1.0f));
    g_state.escape->set_height(.50f);
    g_state.escape->set_width(.50f);
	g_state.escape->set_entity_type(ESCAPE);
	g_state.escape->update(0.0f, NULL, NULL, 0);

	g_state.fire = new Entity[2];
	g_state.fire[0].set_texture_id(fire_texture_id);
    g_state.fire[0].set_position(glm::vec3(2.0f, -2.75f, 0.0f));
	g_state.fire[0].set_scale(glm::vec3(1.0f, 1.0f, 1.0f));
	g_state.fire[0].set_entity_type(FIRE);
	g_state.fire[0].set_height(1.0f);
	g_state.fire[0].set_width(1.0f);
	g_state.fire[0].update(0.0f, NULL, NULL, 0);
    g_state.fire[1].set_texture_id(fire_texture_id);
    g_state.fire[1].set_position(glm::vec3(-2.0f, -2.75f, 0.0f));
    g_state.fire[1].set_scale(glm::vec3(1.0f, 1.0f, 1.0f));
    g_state.fire[1].set_entity_type(FIRE);
    g_state.fire[0].set_height(.7f);
    g_state.fire[0].set_width(.7f);
    g_state.fire[1].update(0.0f, NULL, NULL, 0);


    // ––––– PLAYER (a whale) ––––– //
    GLuint player_texture_id = load_texture(SPRITESHEET_FILEPATH);

    int whale_animations[2][10] =
	{
	    { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 },  // for whale to move to the left,
	    { 10, 11, 12, 13, 14, 15, 16, 17, 18, 19 }   // for whale to move to the right,
	};

    glm::vec3 acceleration = glm::vec3(0.0f, -ACC, 0.0f);

    g_state.player = new Entity(
        player_texture_id,         // texture id
        5.0f,                      // speed
        acceleration,              // acceleration
        3.0f,                      // jumping power
        whale_animations,  // animation index sets
        0.0f,                      // animation time
        10,                         // animation frame amount
        0,                         // current animation index
        10,                         // animation column amount
        2,                         // animation row amount
        .70f,                      // width
        .70f,                       // height
        PLAYER
    );
	g_state.player->set_scale(glm::vec3(0.625f, 1.2f, 1.0f));

    // ––––– GENERAL ––––– //
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void process_input()
{
    g_state.player->set_movement(glm::vec3(0.0f));

    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        switch (event.type) {
            // End game
            case SDL_QUIT:
            case SDL_WINDOWEVENT_CLOSE:
                g_game_is_running = false;
                break;

            case SDL_KEYDOWN:
                switch (event.key.keysym.sym) {
                    case SDLK_q:
                        // Quit the game with a keystroke
                        g_game_is_running = false;
                        break;

                    default:
                        break;
                }

            default:
                break;
        }
    }

    const Uint8 *key_state = SDL_GetKeyboardState(NULL);

    //can control player while you haven't lost or won
    if (!WIN && !LOSE)
    {
        if (key_state[SDL_SCANCODE_SPACE])
        {
            g_state.player->move_up(ACC * .02);
        }

        else
        {
            // Vertical deceleration with gravity when there is no input
            g_state.player->set_acceleration(glm::vec3(g_state.player->get_acceleration().x, -ACC, 0.0f));
        }
        if (key_state[SDL_SCANCODE_LEFT])
        {
            g_state.player->move_left(ACC * .2);
        }
        else if (key_state[SDL_SCANCODE_RIGHT])
        {
            g_state.player->move_right(ACC * .2);
        }
        else
        {
            // Horizontal deceleration when there is no input
            if (g_state.player->get_acceleration().x > 0.0f)
            {
                g_state.player->set_acceleration(glm::vec3(g_state.player->get_acceleration().x - .2 * ACC, g_state.player->get_acceleration().y, 0.0f));
            }
            if (g_state.player->get_acceleration().x < 0.0f)
            {
                g_state.player->set_acceleration(glm::vec3(g_state.player->get_acceleration().x + .2 * ACC, g_state.player->get_acceleration().y, 0.0f));
            }
        }

        if (glm::length(g_state.player->get_acceleration()) > MAXACC)
        {
            g_state.player->normalise_movement();
        }
    }
}

void update()
{
    float ticks = (float)SDL_GetTicks() / MILLISECONDS_IN_SECOND;
    float delta_time = ticks - g_previous_ticks;
    g_previous_ticks = ticks;

    delta_time += g_accumulator;

    if (g_state.player->escape)
    {
        WIN = true;
        g_state.player->set_acceleration(glm::vec3(0.0f, 0.0f, 0.0f));
        g_state.player->set_velocity(glm::vec3(0.0f, 0.0f, 0.0f));
        g_state.player->set_movement(glm::vec3(0.0f, 0.0f, 0.0f));
        g_state.air->m_is_active = false;
    }
	if (g_state.air->get_scale().x <= 0.0f)
	{
		LOSE = true;
        g_state.player->set_movement(glm::vec3(0.0f, 0.0f, 0.0f));
        g_state.air->m_is_active = false;
	}

    if (!g_state.player->m_is_active)
    {
        LOSE = true;
        g_state.air->m_is_active = false;
    }

    if (delta_time < FIXED_TIMESTEP)
    {
        g_accumulator = delta_time;
        return;
    }


    while (delta_time >= FIXED_TIMESTEP)
    {
        g_state.player->update(FIXED_TIMESTEP, NULL, g_state.platforms, PLATFORM_COUNT);
        g_state.player->update(FIXED_TIMESTEP, NULL, g_state.ceilings, CEILING_COUNT);
        g_state.player->update(FIXED_TIMESTEP, NULL, g_state.right_walls, LEFTWALL_COUNT);
        g_state.player->update(FIXED_TIMESTEP, NULL, g_state.left_walls, RIGHTWALL_COUNT);
        g_state.player->update(FIXED_TIMESTEP, NULL, g_state.food, FOOD_COUNT);
		g_state.player->update(FIXED_TIMESTEP, NULL, g_state.fire, 2);
        if (BELLY_FILLED)
        {
            g_state.player->update(FIXED_TIMESTEP, NULL, g_state.escape, 1);
        }
		g_state.air->update(FIXED_TIMESTEP, NULL, NULL, 1);

        delta_time -= FIXED_TIMESTEP;
    }

    g_accumulator = delta_time;
}

void render()
{
    glClear(GL_COLOR_BUFFER_BIT);

    g_state.background->render(&g_program);
	g_state.midground->render(&g_program);

    if (g_state.player->m_is_active)
    {
        g_state.player->render(&g_program);
    }
    
    //always render these
    for (int i = 0; i < RIGHTWALL_COUNT; i++)
    {
        g_state.right_walls[i].render(&g_program);
    }
    for (int i = 0; i < LEFTWALL_COUNT; i++)
    {
        g_state.left_walls[i].render(&g_program);
    }
    for (int i = 0; i < PLATFORM_COUNT; i++)
    {
        g_state.platforms[i].render(&g_program);
    }
    for (int i = 0; i < CEILING_COUNT; i++)
    {
        g_state.ceilings[i].render(&g_program);
    }

	for (int i = 0; i < 2; i++)
	{
		g_state.fire[i].render(&g_program);
	}
    //

    if (g_state.air->m_is_active)
    {
        g_state.air->render(&g_program);
    }
	
	for (int i = 0; i < FOOD_COUNT; i++)
	{
        if (!g_state.food[i].m_is_active)
        {
            FOOD_EATEN++;
            continue;
        }
		g_state.food[i].render(&g_program);
	}

	if (FOOD_EATEN == FOOD_COUNT)
	{
		BELLY_FILLED = true;
	}
    else
    {
		FOOD_EATEN = 0;
    }

    if(BELLY_FILLED)
	{
		g_state.escape->render(&g_program);
	}

    if (WIN)
    {
		g_state.win->render(&g_program);
    }
    if (LOSE)
    {
		g_state.lose->render(&g_program);
    }

    SDL_GL_SwapWindow(g_display_window);
}

void shutdown()
{
    SDL_Quit();

    delete[] g_state.platforms;
    delete[] g_state.ceilings;
    delete[] g_state.right_walls;
    delete[] g_state.left_walls;
	delete[] g_state.food;


    delete g_state.player;
    delete g_state.background;
    delete g_state.midground;
}

// ––––– GAME LOOP ––––– //
int main(int argc, char* argv[])
{
    initialise();

    while (g_game_is_running)
    {
        process_input();
        update();
        render();
    }

    shutdown();
    return 0;
}
