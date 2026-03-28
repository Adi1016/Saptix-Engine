#pragma once

#include <SDL3/SDL.h>
#include <string>

class HubApp
{
public:
    HubApp() = default;
    ~HubApp() = default;

    bool Init();
    void Run();
    void Shutdown();

    std::string GetSelectedProject() const { return selectedProjectPath; }
    bool DidSelectProject() const { return projectSelected; }

private:
    void RenderSplash();
    void RenderHub();
    std::string OpenFolderBrowser();

    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;
    bool running = false;
    
    // Flow State
    bool inSplash = true;
    float bootTimer = 0.0f;
    
    // Output Result
    bool projectSelected = false;
    std::string selectedProjectPath = "";
    
    // Hub specific UI variables
    bool openNewProject = false;
};
