#include "Engine.h"
#include "../PlayerController.h"
#include <SDL3_image/SDL_image.h>
#include <iostream>

// ImGui
#include "imgui.h"
#include "imgui_impl_sdl3.h"
#include "imgui_impl_sdlrenderer3.h"
#include "EditorUI.h"
#include "../Serializer.h"

// INIT
bool Engine::Init(const std::string& projectConfigPath)
{
    if (!SDL_Init(SDL_INIT_VIDEO))
    {
        std::cout << "SDL Init Failed\n";
        return false;
    }

    window = SDL_CreateWindow("Saptix Engine", 1280, 720, SDL_WINDOW_RESIZABLE);
    if (!window)
    {
        std::cout << "Window Creation Failed\n";
        return false;
    }

    renderer = SDL_CreateRenderer(window, nullptr);
    if (!renderer)
    {
        std::cout << "Renderer Failed\n";
        return false;
    }

    // Create an offscreen render target for the game viewport
    renderTarget = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888,
                                     SDL_TEXTUREACCESS_TARGET, 1280, 720);

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    // Unity-style dark theme
    ImGui::StyleColorsDark();
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding    = 4.0f;
    style.FrameRounding     = 3.0f;
    style.GrabRounding      = 3.0f;
    style.TabRounding       = 3.0f;
    style.FramePadding      = ImVec2(6, 4);
    style.ItemSpacing       = ImVec2(8, 6);
    style.Colors[ImGuiCol_WindowBg]        = ImVec4(0.12f, 0.12f, 0.14f, 1.00f);
    style.Colors[ImGuiCol_TitleBg]         = ImVec4(0.08f, 0.08f, 0.09f, 1.00f);
    style.Colors[ImGuiCol_TitleBgActive]   = ImVec4(0.16f, 0.29f, 0.48f, 1.00f);
    style.Colors[ImGuiCol_Header]          = ImVec4(0.20f, 0.40f, 0.68f, 0.80f);
    style.Colors[ImGuiCol_HeaderHovered]   = ImVec4(0.26f, 0.59f, 0.98f, 0.60f);
    style.Colors[ImGuiCol_Button]          = ImVec4(0.20f, 0.40f, 0.68f, 0.80f);
    style.Colors[ImGuiCol_ButtonHovered]   = ImVec4(0.26f, 0.59f, 0.98f, 0.80f);
    style.Colors[ImGuiCol_FrameBg]         = ImVec4(0.20f, 0.20f, 0.22f, 1.00f);
    style.Colors[ImGuiCol_Tab]             = ImVec4(0.14f, 0.14f, 0.16f, 1.00f);
    style.Colors[ImGuiCol_TabHovered]      = ImVec4(0.26f, 0.59f, 0.98f, 0.80f);
    style.Colors[ImGuiCol_TabSelected]     = ImVec4(0.16f, 0.29f, 0.48f, 1.00f);

    ImGui_ImplSDL3_InitForSDLRenderer(window, renderer);
    ImGui_ImplSDLRenderer3_Init(renderer);

    editorUI = new EditorUI();

    playerTexture = IMG_LoadTexture(renderer, "player.png");
    logoTexture = IMG_LoadTexture(renderer, "logo.png");

    if (logoTexture)
    {
        // Set window icon (Requires a surface)
        SDL_Surface* iconSurf = IMG_Load("logo.png");
        if (iconSurf)
        {
            SDL_SetWindowIcon(window, iconSurf);
            SDL_DestroySurface(iconSurf);
        }
    }

    if (playerTexture)
    {
        float w = 0, h = 0;
        SDL_GetTextureSize(playerTexture, &w, &h);
        playerTexW = (int)w;
        playerTexH = (int)h;
    }

    currentProjectPath = projectConfigPath;
    if (!currentProjectPath.empty()) {
        Serializer::LoadScene(scene, currentProjectPath, playerTexture);
    }
    
    state = EngineState::Editor;
    editorMode = true;
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
            ImGui_ImplSDL3_ProcessEvent(&event); // Forward to ImGui FIRST

            if (event.type == SDL_EVENT_QUIT)
                running = false;

            // Toggle Editor Mode with F5 (only in Editor State)
            if (state == EngineState::Editor && event.type == SDL_EVENT_KEY_DOWN && event.key.scancode == SDL_SCANCODE_F5)
                editorMode = !editorMode;
        }

        // Game physics ALWAYS runs — editor never pauses the game
        Update(deltaTime);
        Render();

        SDL_Delay(1);
    }

    ImGui_ImplSDLRenderer3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();
    delete editorUI;
    editorUI = nullptr;
    if (renderTarget)  SDL_DestroyTexture(renderTarget);
    if (playerTexture) SDL_DestroyTexture(playerTexture);
    if (logoTexture)   SDL_DestroyTexture(logoTexture);
}

// UPDATE
void Engine::Update(float deltaTime)
{
    SDL_PumpEvents();

    if (state != EngineState::Editor) return;

    // Camera follows player (Generic component-based search)
    if (GameObject* player = scene.FindObjectWithComponent<PlayerController>())
    {
        camera.position.x = player->position.x - (scene.screenWidth  / 2.0f);
        camera.position.y = player->position.y - (scene.screenHeight / 2.0f);
    }

    camera.Update(deltaTime);       // Tick screen-shake timer
    scene.Update(deltaTime, camera); // Scene can call camera.Shake() on hits
}

// RENDER
void Engine::Render()
{
    // ── Step 1: Render game world into the offscreen texture ──────────────
    SDL_SetRenderTarget(renderer, renderTarget);
    SDL_SetRenderDrawColor(renderer, 25, 25, 50, 255);
    SDL_RenderClear(renderer);
    Vector2 renderCamPos = camera.GetRenderPosition(); // shake-jittered
    scene.Render(renderer, renderCamPos);

    // If editor is open, draw a highlight rect around the selected object
    if (editorMode && editorUI && editorUI->selectedObject)
    {
        GameObject* sel = editorUI->selectedObject;
        SDL_FRect highlightRect = {
            sel->position.x - renderCamPos.x - 3,
            sel->position.y - renderCamPos.y - 3,
            sel->size.x + 6,
            sel->size.y + 6
        };
        SDL_SetRenderDrawColor(renderer, 80, 200, 255, 220);
        SDL_RenderRect(renderer, &highlightRect);
        // Second inner rect for glow effect
        SDL_FRect innerRect = { highlightRect.x + 2, highlightRect.y + 2, highlightRect.w - 4, highlightRect.h - 4 };
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 120);
        SDL_RenderRect(renderer, &innerRect);
    }

    SDL_SetRenderTarget(renderer, nullptr); // Back to screen

    // ── Step 2: If editor is OFF, just blit the game fullscreen ───────────
    if (!editorMode)
    {
        SDL_RenderTexture(renderer, renderTarget, nullptr, nullptr);
        SDL_RenderPresent(renderer);
        return;
    }

    // ── Step 3: Editor mode — clear screen then render ImGui on top ───────
    SDL_SetRenderDrawColor(renderer, 18, 18, 20, 255);
    SDL_RenderClear(renderer);

    ImGui_ImplSDLRenderer3_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();

    if (editorUI)
        editorUI->RenderUI(this, scene, renderer, renderTarget, playerTexture, playerTexW, playerTexH);
        
    ImGui::Render();
    ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), renderer);

    SDL_RenderPresent(renderer);
}

// CLEANUP
void Engine::Shutdown()
{
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}