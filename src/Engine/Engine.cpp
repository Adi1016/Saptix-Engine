#include "Engine.h"
#include "../PlayerController.h"
#include "../AnimatorComponent.h"
#include <SDL3_image/SDL_image.h>
#include <iostream>

// INIT
bool Engine::Init()
{
    if (!SDL_Init(SDL_INIT_VIDEO))
    {
        std::cout << "SDL Init Failed\n";
        return false;
    }

    window = SDL_CreateWindow("Saptix Engine", 800, 600, SDL_WINDOW_RESIZABLE);

    if (!window)
    {
        std::cout << "Window Creation Failed\n";
        return false;
    }

    renderer = SDL_CreateRenderer(window, "software");

    if (!renderer)
    {
        std::cout << "Renderer Failed\n";
        return false;
    }

    SDL_Texture* playerTexture = IMG_LoadTexture(renderer, "player.png");

    if (!playerTexture)
    {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Texture Error", SDL_GetError(), window);
    }
        
    // Populate scene
    scene.objects.push_back({ {100, 100}, {100, 100}, {50,   0} });
    scene.objects.push_back({ {300, 200}, {150, 120}, {0,   40} });
    scene.objects.push_back({ {500, 300}, {80,  80},  {-30, 20} });
        
    for (int i = 0; i < 20; i++)
    {
        scene.objects.push_back({
            { i * 120.0f, 400.0f },
            { 80, 80 },
            { 0, 0 }
        });
    }

    // Attach Component AFTER generating all objects to prevent vector reallocations breaking pointers
    PlayerController* player = new PlayerController();
    player->owner = &scene.objects[0];
    scene.objects[0].components.push_back(player);

    AnimatorComponent* animator = new AnimatorComponent();
    animator->owner = &scene.objects[0];
    animator->LoadAnimFile("player.sanim");
    scene.objects[0].components.push_back(animator);

    // Apply texture
    scene.objects[0].texture = playerTexture;

    running = true;
    return true;
}

// RUN LOOP
void Engine::Run()
{
    Uint64 lastTime = SDL_GetPerformanceCounter();

    while (running)
    {
        Uint64 currentTime = SDL_GetPerformanceCounter();
        float deltaTime = (currentTime - lastTime) / (float)SDL_GetPerformanceFrequency();
        lastTime = currentTime;

        if (deltaTime > 0.1f)
            deltaTime = 0.1f;

        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_EVENT_QUIT)
                running = false;
        }

        Update(deltaTime);
        Render();

        SDL_Delay(1);
    }
}

// UPDATE
void Engine::Update(float deltaTime)
{
    SDL_PumpEvents();

    // Camera -- follow player (center screen on objects[0])
    if (!scene.objects.empty())
    {
        camera.position.x = scene.objects[0].position.x - (scene.screenWidth  / 2.0f);
        camera.position.y = scene.objects[0].position.y - (scene.screenHeight / 2.0f);
    }

    // Scene handles all physics
    scene.Update(deltaTime);
}

// RENDER
void Engine::Render()
{
    SDL_SetRenderDrawColor(renderer, 25, 25, 50, 255);
    SDL_RenderClear(renderer);

    // Scene handles all rendering
    scene.Render(renderer, camera.position);

    SDL_RenderPresent(renderer);
}

// CLEANUP
void Engine::Shutdown()
{
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}