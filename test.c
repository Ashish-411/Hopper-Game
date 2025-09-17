#include <stdio.h>
#include <SDL2/SDL.h>
#include "constants.h"
#include <time.h>
#include <stdlib.h>
#include <stdbool.h>

int last_frame_time = 0;
typedef enum
{
    JUMPING_UP,
    FALLING_DOWN,
    ON_GROUND
} HopperState;

typedef struct
{
    float x;
    float y;
    float dx;
    float dy;
    float initial_y; // Initial position when jumping
    bool jumping;    // Flag to indicate if the hopper is currently jumping
} player;

player platform[No_platform];
player hopper;
player base_platform;
HopperState hopper_state = JUMPING_UP;

int gap_between_platform(int x, int y)
{
    for (int i = 0; i < No_platform; i++)
    {
        if (abs(x - platform[i].x) < PLATFORM_GAP &&
            abs(y - platform[i].y) < PLATFORM_GAP)
        {
            return TRUE; // Too close to existing platform
        }
    }
    return FALSE; // Not too close to any existing platform
}

int Intersecting_platform(int x, int y)
{
    for (int i = 0; i < No_platform; i++)
    {
        if (x < platform[i].x + Platform_width &&
            x + Platform_width > platform[i].x &&
            y < platform[i].y + Platform_height &&
            y + Platform_height > platform[i].y)
        {
            return TRUE; // intersected
        }
    }
    return FALSE; // no intersected
}

void generaterPlatform()
{
    int x, y;
    srand(time(NULL));
    for (int i = 0; i < No_platform; i++)
    {
        do
        {
            x = rand() % (Window_width - Platform_width);
            y = rand() % (Window_height - Platform_height);
        } while (Intersecting_platform(x, y) || gap_between_platform(x, y));
        platform[i].x = x;
        platform[i].y = y;
    }
}

void setup()
{
    generaterPlatform();
    // Set base platform position at the middle bottom of the window
    base_platform.x = (Window_width - Platform_width) / 2;
    base_platform.y = Window_height - Platform_height;

    // Adjust hopper position to be on the base platform
    hopper.x = base_platform.x + (Platform_width - Hopper_width) / 2;
    hopper.y = base_platform.y - Hopper_height;
    hopper.initial_y = hopper.y; // Store initial position
    hopper.jumping = true;       // Start jumping

    // Set initial upward velocity for jump
    hopper.dy = -HOPPER_JUMP_SPEED;
}

int game_is_running = TRUE; // Start the game
SDL_Window *window = NULL;
SDL_Renderer *renderer = NULL;

int window_initialize(void)
{
    if (SDL_Init(SDL_INIT_EVERYTHING) != 0)
    {
        fprintf(stderr, "Error initializing SDL");
        return FALSE;
    }
    window = SDL_CreateWindow(NULL, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, Window_width, Window_height, SDL_WINDOW_BORDERLESS);
    if (!window)
    {
        fprintf(stderr, "Error creating SDL window");
        return FALSE;
    }
    renderer = SDL_CreateRenderer(window, -1, 0);
    if (!renderer)
    {
        fprintf(stderr, "Error creating SDL renderer");
        return FALSE;
    }
    return TRUE;
}

void renderPlatform()
{
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
    for (int i = 0; i < No_platform; i++)
    {
        SDL_Rect platformrect = {
            (int)platform[i].x,
            (int)platform[i].y,
            (int)Platform_width,
            (int)Platform_height

        };
        SDL_RenderFillRect(renderer, &platformrect);
    }
}

void basePlatform()
{
    SDL_SetRenderDrawColor(renderer, 255, 0, 0, 0);

    SDL_Rect baseplatformrect = {
        (int)base_platform.x,
        (int)base_platform.y,
        (int)Platform_width,
        (int)Platform_height

    };
    SDL_RenderFillRect(renderer, &baseplatformrect);
}

void renderHopper()
{
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
    SDL_Rect Hopperrect = {
        (int)hopper.x,
        (int)hopper.y,
        (int)Hopper_width,
        (int)Hopper_height};
    SDL_RenderFillRect(renderer, &Hopperrect);
}

void process_input()
{
    SDL_Event event;
    SDL_PollEvent(&event);
    switch (event.type)
    {
    case SDL_QUIT:
        game_is_running = FALSE;
        break;

    case SDL_KEYDOWN:
        switch (event.key.keysym.sym)
        {
        case SDLK_ESCAPE:
            game_is_running = FALSE;
            break;
        case SDLK_RIGHT:
            hopper.x += 50;
            break;
        case SDLK_LEFT:
            hopper.x -= 50;
            break;
        default:
            break;
        }
        break;
    }
}

void update()
{
    // Adjusting the time delay with FPS and Pixel per second with delta time
    int time_to_wait = FRAME_TARGET_TIME - (SDL_GetTicks() - last_frame_time);
    if (time_to_wait > 0 && time_to_wait <= FRAME_TARGET_TIME)
    {
        SDL_Delay(time_to_wait);
    }
    float delta_time = (SDL_GetTicks() - last_frame_time) / 1000.0f;
    last_frame_time = SDL_GetTicks();
    // Environment movement
    for (int i = 0; i < No_platform; i++)
    {
        platform[i].y += PLATFORM_SPEED * delta_time; // Move each platform downwards
        // If a platform reaches the bottom of the window, reset its position above the window
        if (platform[i].y >= Window_height)
        {
            platform[i].y = -Platform_height;                         // Reset the platform position above the window
            platform[i].x = rand() % (Window_width - Platform_width); // Randomize horizontal position
            // Adjust platform horizontal position to ensure equal spacing
            for (int j = 0; j < i; j++)
            {
                if (abs(platform[i].x - platform[j].x) < PLATFORM_GAP)
                {
                    platform[i].x += PLATFORM_GAP;
                }
            }
        }
    }

    // Hopper movement logic
    if (hopper_state == JUMPING_UP)
    {
        hopper.y += hopper.dy * delta_time;

        // Check if hopper reaches the maximum jump height
        if (hopper.y <= hopper.initial_y - JUMP_HEIGHT)
        {
            hopper_state = FALLING_DOWN; // Change state to falling down
        }

        // Check if hopper reaches the ground
        if (hopper.y >= Window_height - Hopper_height)
        {
            game_is_running = FALSE;
        }
    }
    else if (hopper_state == FALLING_DOWN)
    {
        hopper.y += GRAVITY * delta_time;

        // Check if hopper reaches the ground
        if (hopper.y >= Window_height - Hopper_height)
        {
            game_is_running = FALSE;
        }
    }

    // Check for collisions and handle jumping
    if (hopper_state == FALLING_DOWN)
    {
        for (int i = 0; i < No_platform; i++)
        {
            if (hopper.x < platform[i].x + Platform_width &&
                hopper.x + Hopper_width > platform[i].x &&
                hopper.y < platform[i].y + Platform_height &&
                hopper.y + Hopper_height > platform[i].y)
            {
                // Collision detected, initiate jump
                hopper.dy = -HOPPER_JUMP_SPEED;
                hopper_state = JUMPING_UP;
                break; // Exit loop since hopper can only collide with one platform at a time
            }
        }
        for (int i = 0; i < No_platform; i++)
        {
            if (hopper.x < platform[i].x + Platform_width &&
                hopper.x + Hopper_width > platform[i].x &&
                hopper.y < platform[i].y + Platform_height &&
                hopper.y + Hopper_height > platform[i].y)
            {
                // The hopper intersects with a platform while falling,
                // initiate environment movement

                base_platform.y += PLATFORM_SPEED * delta_time;
                break; // Exit loop since hopper can only intersect with one platform at a time
            }
        }
    }
}

void render()
{
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderClear(renderer);
    renderPlatform();
    basePlatform();
    renderHopper();

    SDL_RenderPresent(renderer);
}

void destroy()
{
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}

int main(int argc, char *argv[])
{
    game_is_running = window_initialize();
    setup();
    while (game_is_running)
    {
        process_input();
        update();
        render();
    }
    destroy();

    return 0;
}
