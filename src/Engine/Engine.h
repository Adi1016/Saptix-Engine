#pragma once
#include <SDL3/SDL.h>
#include "../Scene.h"
#include "../Camera.h"

// Forward declaration — full definition lives in EditorUI.h, included only by Engine.cpp
class EditorUI;

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
    bool          editorMode = false;

    Scene  scene;
    Camera camera{};
    EditorUI*    editorUI    = nullptr;   // pointer — avoids incomplete type
    SDL_Texture* renderTarget  = nullptr;
    SDL_Texture* playerTexture = nullptr;
    int playerTexW = 0, playerTexH = 0;
};