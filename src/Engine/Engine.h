#pragma once
#include <SDL3/SDL.h>
#include "../Scene.h"
#include "../Camera.h"

enum class EngineState {
    SplashScreen,
    ProjectHub,
    Editor
};

// Forward declaration — full definition lives in EditorUI.h, included only by Engine.cpp
class EditorUI;

class Engine
{
public:
    bool Init();
    void Run();
    void Shutdown();

    // Changed to public so EditorUI can read/write the state and paths directly from Engine* pointer if passed,
    // or we just keep them private and pass by reference to RenderUI/RenderHub
    EngineState   state = EngineState::SplashScreen;
    std::string   currentProjectPath = "";
    float         bootTimer = 0.0f;

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