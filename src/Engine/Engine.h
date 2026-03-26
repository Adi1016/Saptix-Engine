#pragma once
#include <SDL3/SDL.h>
#include "../Scene.h"
#include "../Camera.h"

class Engine
{
public:
    bool Init();
    void Run();
    void Shutdown();

private:
    void Update(float deltaTime);
    void Render();

    SDL_Window*   window   = nullptr;
    SDL_Renderer* renderer = nullptr;
    bool          running  = false;

    Scene  scene;
    Camera camera{};
};