#include "HubApp.h"
#include <iostream>
#include <vector>
#include <SDL3_image/SDL_image.h>
#include <direct.h>   // For _mkdir

// ImGui
#include "imgui.h"
#include "imgui_impl_sdl3.h"
#include "imgui_impl_sdlrenderer3.h"

#include "../Scene.h"
#include "../Serializer.h"

#define NOMINMAX
#include <windows.h>  // For COM IFileDialog
#include <shobjidl.h>

#pragma comment(lib, "ole32.lib")

std::string HubApp::OpenFolderBrowser()
{
    std::string outPath = "";
    IFileDialog* pfd = nullptr;
    if (SUCCEEDED(CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pfd))))
    {
        DWORD dwOptions;
        if (SUCCEEDED(pfd->GetOptions(&dwOptions)))
            pfd->SetOptions(dwOptions | FOS_PICKFOLDERS | FOS_FORCEFILESYSTEM);

        if (SUCCEEDED(pfd->Show(NULL))) // NULL hwnd makes it unparented
        {
            IShellItem* psi;
            if (SUCCEEDED(pfd->GetResult(&psi)))
            {
                PWSTR pszPath;
                if (SUCCEEDED(psi->GetDisplayName(SIGDN_FILESYSPATH, &pszPath)))
                {
                    std::wstring ws(pszPath);
                    outPath = std::string(ws.begin(), ws.end());
                    CoTaskMemFree(pszPath);
                }
                psi->Release();
            }
        }
        pfd->Release();
    }
    return outPath;
}

bool HubApp::Init()
{
    if (!SDL_Init(SDL_INIT_VIDEO))
    {
        std::cout << "Hub SDL Init Failed\n";
        return false;
    }

    // Completely borderless window for Splash and Hub (as requested)
    window = SDL_CreateWindow("Saptix Hub Launcher", 1280, 720, SDL_WINDOW_BORDERLESS | SDL_WINDOW_HIDDEN);
    if (!window)
    {
        std::cout << "Hub Window Creation Failed\n";
        return false;
    }

    renderer = SDL_CreateRenderer(window, nullptr);
    if (!renderer)
    {
        std::cout << "Hub Renderer Failed\n";
        return false;
    }

    // Center window and show
    SDL_SetWindowPosition(window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
    SDL_ShowWindow(window);

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    // Unity-style dark theme as base
    ImGui::StyleColorsDark();
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding    = 0.0f; // Flat for borderless
    style.FrameRounding     = 3.0f;
    style.GrabRounding      = 3.0f;
    style.TabRounding       = 3.0f;
    style.FramePadding      = ImVec2(6, 4);
    style.ItemSpacing       = ImVec2(8, 6);
    style.Colors[ImGuiCol_WindowBg]        = ImVec4(0.12f, 0.12f, 0.14f, 1.00f);
    
    ImGui_ImplSDL3_InitForSDLRenderer(window, renderer);
    ImGui_ImplSDLRenderer3_Init(renderer);

    // Load logo
    logoTexture = IMG_LoadTexture(renderer, "logo.png");
    if (logoTexture)
    {
        SDL_Surface* iconSurf = IMG_Load("logo.png");
        if (iconSurf)
        {
            SDL_SetWindowIcon(window, iconSurf);
            SDL_DestroySurface(iconSurf);
        }
    }

    running = true;
    inSplash = true;
    bootTimer = 0.0f;
    projectSelected = false;

    return true;
}

void HubApp::Run()
{
    Uint64 lastTime = SDL_GetPerformanceCounter();

    while (running)
    {
        Uint64 currentTime = SDL_GetPerformanceCounter();
        float deltaTime = (currentTime - lastTime) / (float)SDL_GetPerformanceFrequency();
        lastTime = currentTime;
        if (deltaTime > 0.1f) deltaTime = 0.1f;

        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            ImGui_ImplSDL3_ProcessEvent(&event);
            if (event.type == SDL_EVENT_QUIT) {
                running = false;
            }
        }

        SDL_SetRenderDrawColor(renderer, 19, 19, 19, 255); // #131313 Dark BG
        SDL_RenderClear(renderer);

        ImGui_ImplSDLRenderer3_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();

        if (inSplash)
        {
            RenderSplash();
            bootTimer += deltaTime;
            if (bootTimer > 2.0f) // Show splash for 2s
            {
                inSplash = false;
            }
        }
        else
        {
            RenderHub();
        }

        ImGui::Render();
        ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), renderer);

        SDL_RenderPresent(renderer);
        SDL_Delay(1);
    }
}

void HubApp::Shutdown()
{
    ImGui_ImplSDLRenderer3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();

    if (logoTexture) SDL_DestroyTexture(logoTexture);
    if (renderer) SDL_DestroyRenderer(renderer);
    if (window) SDL_DestroyWindow(window);
    
    // Do NOT call SDL_Quit() here! Because the Engine will use it immediately after.
    // SDL supports multiple window creation/destruction safely within initialized boundaries.
}

void HubApp::RenderSplash()
{
    ImGuiIO& io = ImGui::GetIO();
    float sw = io.DisplaySize.x;
    float sh = io.DisplaySize.y;

    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImVec2(sw, sh));
    ImGui::Begin("Splash", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoInputs);

    ImDrawList* dl = ImGui::GetWindowDrawList();

    // 1. Grid Background
    ImU32 gridCol = IM_COL32(186, 203, 185, 8); // #bacbb9
    float gridSpace = 40.0f;
    for (float x = 0; x < sw; x += gridSpace) dl->AddLine(ImVec2(x, 0), ImVec2(x, sh), gridCol, 1.0f);
    for (float y = 0; y < sh; y += gridSpace) dl->AddLine(ImVec2(0, y), ImVec2(sw, y), gridCol, 1.0f);

    // 2. Neon Corner Decor
    ImU32 borderCol = IM_COL32(133, 149, 133, 50); // #859585 outline 20%
    dl->AddRect(ImVec2(40, 40), ImVec2(168, 168), borderCol, 0, 0, 1.0f);
    dl->AddRect(ImVec2(sw - 168, 40), ImVec2(sw - 40, 168), borderCol, 0, 0, 1.0f);
    
    dl->AddRectFilled(ImVec2(0, sh/2.0f - 48.0f), ImVec2(4, sh/2.0f + 48.0f), IM_COL32(117, 255, 158, 100)); // Left bar
    dl->AddRectFilled(ImVec2(sw - 4, sh/2.0f - 48.0f), ImVec2(sw, sh/2.0f + 48.0f), IM_COL32(255, 82, 95, 100)); // Right bar

    // 3. Central "SYSTEM INITIALIZE" Anchor
    float cx = sw / 2.0f;
    float cy = sh / 2.0f;
    
    ImU32 primaryCol = IM_COL32(117, 255, 158, 255); // #75ff9e
    ImU32 pinkCol    = IM_COL32(255, 82, 95, 255);   // #ff525f
    
    float topY = cy - 130.0f;
    dl->AddLine(ImVec2(cx - 160, topY), ImVec2(cx - 105, topY), IM_COL32(133, 149, 133, 100), 2.0f);
    ImVec2 initSize = ImGui::CalcTextSize("S Y S T E M   I N I T I A L I Z E");
    dl->AddText(ImVec2(cx - initSize.x/2.0f, topY - initSize.y/2.0f), primaryCol, "S Y S T E M   I N I T I A L I Z E");
    dl->AddLine(ImVec2(cx + 105, topY), ImVec2(cx + 160, topY), IM_COL32(133, 149, 133, 100), 2.0f);

    // 4. Logo Glow & Image
    float logoSize = 160.0f;
    float logoX = cx - logoSize / 2.0f;
    float logoY = cy - 100.0f;

    if (logoTexture)
    {
        // Glow effect behind logo
        ImU32 primaryGlow = IM_COL32(117, 255, 158, 40);
        for (float off = 2.0f; off <= 8.0f; off += 2.0f) {
            dl->AddRectFilled(ImVec2(logoX - off, logoY - off), ImVec2(logoX + logoSize + off, logoY + logoSize + off), primaryGlow, 8.0f);
        }
        dl->AddImage(ImTextureRef{logoTexture}, ImVec2(logoX, logoY), ImVec2(logoX + logoSize, logoY + logoSize));
    }
    else
    {
        // Fallback to text if texture fails
        ImGui::SetWindowFontScale(3.5f);
        ImVec2 saptSize = ImGui::CalcTextSize("Sapt");
        ImVec2 ixSize = ImGui::CalcTextSize("ix");
        float totalLogoW = saptSize.x + ixSize.x;
        dl->AddText(ImVec2(cx - totalLogoW / 2.0f, cy - 70.0f), primaryCol, "Sapt");
        dl->AddText(ImVec2(cx - totalLogoW / 2.0f + saptSize.x, cy - 70.0f), pinkCol, "ix");
        ImGui::SetWindowFontScale(1.0f);
    }

    // 5. ENGINE Bar
    ImGui::SetWindowFontScale(1.5f);
    ImVec2 engSize = ImGui::CalcTextSize("E N G I N E");
    float engY = logoY + logoSize + 10.0f;
    dl->AddText(ImVec2(cx - engSize.x/2.0f, engY), IM_COL32(229, 226, 225, 255), "E N G I N E");
    dl->AddLine(ImVec2(cx - engSize.x/2.0f - 80.0f, engY + engSize.y/2.0f), ImVec2(cx - engSize.x/2.0f - 15.0f, engY + engSize.y/2.0f), borderCol, 2.0f);
    dl->AddLine(ImVec2(cx + engSize.x/2.0f + 15.0f, engY + engSize.y/2.0f), ImVec2(cx + engSize.x/2.0f + 80.0f, engY + engSize.y/2.0f), borderCol, 2.0f);
    ImGui::SetWindowFontScale(1.0f);

    // 6. Parameters
    float pY = engY + 50.0f;
    ImU32 dimText = IM_COL32(186, 203, 185, 255);
    ImU32 whiteText = IM_COL32(229, 226, 225, 255);
    
    dl->AddLine(ImVec2(cx - 150, pY), ImVec2(cx - 150, pY + 35), IM_COL32(117, 255, 158, 80), 2.0f);
    dl->AddText(ImVec2(cx - 140, pY), dimText, "GRAPHICS API");
    dl->AddText(ImVec2(cx - 140, pY + 15), primaryCol, "VULKAN 1.3");

    dl->AddLine(ImVec2(cx - 30, pY), ImVec2(cx - 30, pY + 35), borderCol, 2.0f);
    dl->AddText(ImVec2(cx - 20, pY), dimText, "PHYSICS");
    dl->AddText(ImVec2(cx - 20, pY + 15), whiteText, "MULTITHREADED");

    dl->AddLine(ImVec2(cx + 90, pY), ImVec2(cx + 90, pY + 35), borderCol, 2.0f);
    dl->AddText(ImVec2(cx + 100, pY), dimText, "ASSET PIPELINE");
    dl->AddText(ImVec2(cx + 100, pY + 15), whiteText, "READY");

    // 7. Info Bottom anchors
    dl->AddText(ImVec2(40, sh - 100), dimText, "(C) 2026 SAPTIX INTERACTIVE");
    dl->AddText(ImVec2(40, sh - 85), dimText, "ALL RUNTIME RIGHTS RESERVED");

    ImVec2 bI_Size = ImGui::CalcTextSize("BUILD INTEGRITY");
    dl->AddText(ImVec2(sw - 160, sh - 105), dimText, "BUILD INTEGRITY");
    dl->AddRectFilled(ImVec2(sw - 160, sh - 85), ImVec2(sw - 40, sh - 60), IM_COL32(53, 53, 52, 255));
    dl->AddText(ImVec2(sw - 150, sh - 78), primaryCol, "(+)");
    dl->AddText(ImVec2(sw - 120, sh - 78), whiteText, "v4.2.0 Stable");

    // 8. The Progress Bar
    float pbWidth = 600.0f;
    float pbX = cx - pbWidth/2.0f;
    float pbY = sh - 140.0f;

    // If you want the loader slower to match the old 10s wait, change 2.0f to bootTimer/10.0f inside prct
    float prct = bootTimer / 2.0f; // Launcher now waits 2s 
    if (prct > 1.0f) prct = 1.0f;
    
    // Label above
    dl->AddText(ImVec2(pbX, pbY - 20), primaryCol, "LOADING CORE MANIFEST...");
    
    char pctStr[32];
    snprintf(pctStr, sizeof(pctStr), "%d%%", (int)(prct * 100.0f));
    ImGui::SetWindowFontScale(2.5f);
    ImVec2 pctSize = ImGui::CalcTextSize(pctStr);
    dl->AddText(ImVec2(pbX + pbWidth - pctSize.x, pbY - pctSize.y - 5.0f), whiteText, pctStr);
    ImGui::SetWindowFontScale(1.0f);

    // Track background
    ImU32 trackCol = IM_COL32(53, 53, 52, 255);
    dl->AddRectFilled(ImVec2(pbX, pbY), ImVec2(pbX + pbWidth, pbY + 6.0f), trackCol);
    
    // Progress fill (Gradient effect mapped as a solid fill with tip)
    dl->AddRectFilled(ImVec2(pbX, pbY), ImVec2(pbX + (pbWidth * prct), pbY + 6.0f), primaryCol);
    // Tip glow
    if (prct > 0.05f)
        dl->AddRectFilled(ImVec2(pbX + (pbWidth * prct) - 40.0f, pbY), ImVec2(pbX + (pbWidth * prct), pbY + 6.0f), IM_COL32(255, 255, 255, 100));

    // Segment chunks (Draw vertical lines to 'cut' the bar)
    for (float sx = pbX + 40.0f; sx < pbX + pbWidth; sx += 75.0f) {
        dl->AddLine(ImVec2(sx, pbY), ImVec2(sx, pbY + 6.0f), IM_COL32(19, 19, 19, 255), 1.0f);
    }

    // Under-text
    dl->AddText(ImVec2(pbX, pbY + 12), dimText, "KERNEL_INIT: SUCCESS (0.42ms)");
    dl->AddText(ImVec2(pbX + pbWidth - 140, pbY + 12), dimText, "MEMORY_MAP: 0xFF003A...");
    
    ImGui::End();
}

void HubApp::RenderHub()
{
    ImGuiIO& io = ImGui::GetIO();
    float sw = io.DisplaySize.x;
    float sh = io.DisplaySize.y;

    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImVec2(sw, sh));

    ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings;
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0)); // Kill default padding
    ImGui::PushStyleColor(ImGuiCol_WindowBg, IM_COL32(19, 19, 19, 255)); // #131313 Main Surface
    ImGui::Begin("Saptix Hub", nullptr, flags);

    // --- Custom Colors ---
    ImU32 colPrimary    = IM_COL32(117, 255, 158, 255); // #75ff9e
    ImU32 colBgHover    = IM_COL32(42, 42, 42, 255);    // #2a2a2a
    ImU32 colCardBg     = IM_COL32(53, 53, 52, 255);    // #353534
    ImU32 colTextDim    = IM_COL32(186, 203, 185, 130); // #bacbb9 opacity
    ImU32 colError      = IM_COL32(255, 82, 95, 255);   // #ff525f
    
    float sidebarW = 260.0f;
    
    // ============================================
    // 1. SIDEBAR
    // ============================================
    ImGui::PushStyleColor(ImGuiCol_ChildBg, IM_COL32(28, 27, 27, 255)); // #1c1b1b
    ImGui::BeginChild("Sidebar", ImVec2(sidebarW, 0), false, ImGuiWindowFlags_NoScrollbar);
    
    ImGui::Dummy(ImVec2(0, 24)); // top padding
    if (logoTexture)
    {
        ImGui::SetCursorPosX(24);
        ImGui::Image(ImTextureRef{logoTexture}, ImVec2(64, 64));
        ImGui::Dummy(ImVec2(0, 12));
    }
    ImGui::Indent(24);
    
    ImGui::SetWindowFontScale(1.8f);
    ImGui::TextColored(ImVec4(1,1,1,1), "Saptix Engine");
    ImGui::SetWindowFontScale(0.7f);
    ImGui::TextColored(ImVec4(0.7f, 0.8f, 0.7f, 0.5f), "V4.2.0-STABLE");
    ImGui::SetWindowFontScale(1.0f);
    ImGui::Dummy(ImVec2(0, 24));

    // + New Project Button
    ImGui::PushStyleColor(ImGuiCol_Button, colPrimary);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(98, 255, 150, 255));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, IM_COL32(0, 228, 117, 255));
    ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(0, 57, 24, 255)); // #003918
    if (ImGui::Button("+ NEW PROJECT", ImVec2(sidebarW - 48, 40))) {
        openNewProject = true;
    }
    ImGui::PopStyleColor(4);
    ImGui::Dummy(ImVec2(0, 24));
    ImGui::Unindent(24);
    
    // Navigation Options
    ImGui::PushStyleColor(ImGuiCol_Header, colBgHover);
    ImGui::PushStyleColor(ImGuiCol_HeaderHovered, colBgHover);
    ImGui::PushStyleColor(ImGuiCol_Text, colPrimary);
    ImGui::Selectable("  [F]  PROJECTS", true, 0, ImVec2(sidebarW, 40));
    ImGui::GetWindowDrawList()->AddRectFilled(ImGui::GetItemRectMin(), ImVec2(ImGui::GetItemRectMin().x + 4, ImGui::GetItemRectMax().y), colPrimary);
    ImGui::PopStyleColor(); // primary text

    ImGui::PushStyleColor(ImGuiCol_Text, colTextDim);
    ImGui::Selectable("  [D]  INSTALLS", false, 0, ImVec2(sidebarW, 40));
    ImGui::Selectable("  [S]  LEARN", false, 0, ImVec2(sidebarW, 40));
    ImGui::Selectable("  [G]  COMMUNITY", false, 0, ImVec2(sidebarW, 40));
    ImGui::PopStyleColor(3); // text, header, hover
    
    // Sidebar Bottom Footer Anchors
    float footY = ImGui::GetWindowHeight() - 140.0f;
    ImGui::SetCursorPosY(footY);
    ImGui::Indent(24);
    ImGui::PushStyleColor(ImGuiCol_Text, colTextDim);
    ImGui::Text("  [ * ]  SETTINGS"); ImGui::Dummy(ImVec2(0, 10));
    ImGui::Text("  [ ! ]  FEEDBACK");
    ImGui::PopStyleColor();
    ImGui::Dummy(ImVec2(0, 10));
    
    ImGui::EndChild();
    ImGui::PopStyleColor(); // Sidebar BG

    // ============================================
    // 2. MAIN WORKSPACE
    // ============================================
    ImGui::SameLine(0, 0);
    ImGui::BeginChild("MainCanvas", ImVec2(0, 0), false, ImGuiWindowFlags_NoScrollbar);
    
    // Header
    ImGui::BeginChild("TopHeader", ImVec2(0, 64), false, ImGuiWindowFlags_NoScrollbar);
    
    // Allow closing the launcher from the header since borderless has no native X button
    ImGui::SetCursorScreenPos(ImVec2(sw - 40, 16));
    ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(0,0,0,0));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(255,50,50,255));
    if (ImGui::Button("X", ImVec2(32, 32))) {
        running = false;
    }
    ImGui::PopStyleColor(2);
    ImGui::EndChild();
    
    // Scrollable Canvas
    ImGui::PushStyleColor(ImGuiCol_ChildBg, IM_COL32(14, 14, 14, 255)); // #0e0e0e Canvas lowest
    ImGui::BeginChild("ScrollContent", ImVec2(0, 0), false);
    ImGui::Indent(32);
    ImGui::Dummy(ImVec2(0, 32));

    // -- HERO TEXT --
    ImGui::SetWindowFontScale(4.0f);
    ImGui::TextColored(ImVec4(1,1,1,1), "PROJECTS");
    ImGui::SetWindowFontScale(1.0f);
    ImGui::TextColored(ImVec4(0.6f, 0.7f, 0.6f, 1.0f), "Active workspaces and deployed environments.");
    ImGui::Dummy(ImVec2(0, 48));

    // ============================================
    // BENTO GRID CARDS (DYNAMIC RECENT PROJECTS)
    // ============================================
    float availW = ImGui::GetWindowWidth();
    float cw1 = (availW - 100) / 3.0f; // Card width for 3 cols
    float tallH = 220.0f;
    
    ImVec2 gridStart = ImGui::GetCursorScreenPos();
    
    std::vector<std::string> recents = Serializer::GetRecentProjects();
    int rcIndex = 0;
    
    for (const auto& path : recents) {
        float xOffset = (rcIndex % 3) * (cw1 + 16.0f);
        float yOffset = (rcIndex / 3) * (tallH + 24.0f);
        ImVec2 fMin = ImVec2(gridStart.x + xOffset, gridStart.y + yOffset); 
        ImVec2 fMax = ImVec2(gridStart.x + xOffset + cw1, gridStart.y + yOffset + tallH);
        
        // Draw Interactive Button background for Recent System File
        ImGui::SetCursorScreenPos(fMin);
        ImGui::PushStyleColor(ImGuiCol_Button, colCardBg);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(80, 80, 80, 255));
        if (ImGui::Button(("##"+path).c_str(), ImVec2(cw1, tallH))) {
            // LOAD PROJECT HIT
            selectedProjectPath = path;
            projectSelected = true;
            running = false;
        }
        ImGui::PopStyleColor(2);
        
        // Extract a display name (folder name)
        std::string dispName = "LOCAL PROJECT";
        size_t lastSlash = path.find_last_of("\\/");
        if (lastSlash != std::string::npos && lastSlash > 0) {
            size_t prevSlash = path.find_last_of("\\/", lastSlash - 1);
            if (prevSlash != std::string::npos) {
                dispName = path.substr(prevSlash + 1, lastSlash - prevSlash - 1);
            } else {
                dispName = path.substr(lastSlash + 1);
            }
        }
        
        // Draw Visuals on top
        ImVec2 txtPos = fMin;
        ImGui::GetWindowDrawList()->AddRectFilledMultiColor(fMin, ImVec2(fMax.x, fMin.y + 70), IM_COL32(30, 40, 50, 255), IM_COL32(20, 30, 40, 255), colCardBg, colCardBg);
        ImGui::GetWindowDrawList()->AddText(ImGui::GetFont(), 20.0f, ImVec2(txtPos.x + 20, txtPos.y + 80), IM_COL32(255,255,255,255), dispName.c_str());
        ImGui::GetWindowDrawList()->AddText(ImGui::GetFont(), 12.0f, ImVec2(txtPos.x + 20, txtPos.y + 110), colTextDim, path.c_str());
        
        rcIndex++;
    }

    ImGui::Dummy(ImVec2(0, 400)); // Buffer scroll width
    ImGui::EndChild(); // End Custom Scroll Region
    ImGui::PopStyleColor(); // Pop Canvas lowest ChildBg

    ImGui::EndChild(); // MainCanvas Child

    // ============================================
    // CREATE WORKSPACE MODAL
    // ============================================
    if (openNewProject) ImGui::OpenPopup("INITIALIZE WORKSPACE");
    
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(24, 24));
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10, 10));
    ImGui::PushStyleColor(ImGuiCol_PopupBg, IM_COL32(25, 25, 27, 255));
    ImGui::PushStyleColor(ImGuiCol_FrameBg, IM_COL32(15, 15, 15, 255));

    if (ImGui::BeginPopupModal("INITIALIZE WORKSPACE", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings))
    {
        ImGui::SetWindowFontScale(1.4f);
        ImGui::TextColored(ImVec4(0.46f, 1.0f, 0.62f, 1.0f), "NEW WORKSPACE");
        ImGui::SetWindowFontScale(1.0f);
        ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "Configure your new engine environment.");
        ImGui::Dummy(ImVec2(0, 16));

        static char projName[128] = "MyNewGame";
        static char projPath[512] = "D:\\MY WORK"; // Default workspace
        
        ImGui::Text("Workspace Name");
        ImGui::PushItemWidth(400);
        ImGui::InputText("##ProjName", projName, IM_ARRAYSIZE(projName));
        
        ImGui::Dummy(ImVec2(0, 8));
        ImGui::Text("Location Path (Engine Data)");
        ImGui::InputText("##Location", projPath, IM_ARRAYSIZE(projPath));
        ImGui::PopItemWidth();
        
        ImGui::SameLine();
        if (ImGui::Button("...##Browse", ImVec2(40, 0)))
        {
            std::string chosen = OpenFolderBrowser();
            if (!chosen.empty())
            {
                snprintf(projPath, sizeof(projPath), "%s", chosen.c_str());
            }
        }
        
        ImGui::Dummy(ImVec2(0, 32));
        
        // Align buttons right
        ImGui::SetCursorPosX(ImGui::GetWindowWidth() - 250);
        
        if (ImGui::Button("CANCEL", ImVec2(100, 40))) {
            openNewProject = false;
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        
        ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(117, 255, 158, 255));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(98, 255, 150, 255));
        ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(0, 0, 0, 255));
        if (ImGui::Button("INITIALIZE", ImVec2(120, 40)))
        {
            // Formulate full directory path: D:\MY WORK\MyNewGame
            std::string fullDirPath = std::string(projPath) + "\\" + std::string(projName);
            
            // Create the directory using windows direct.h
            _mkdir(fullDirPath.c_str());

            // Save initial scene file inside that newly created folder
            std::string fullScenePath = fullDirPath + "\\scene.saptix";
            
            // Create empty scene and save it here instead of passing engine
            Scene tempScene;
            Serializer::SaveScene(tempScene, fullScenePath);
            
            selectedProjectPath = fullScenePath;
            projectSelected = true;
            running = false; // Exit app!
            
            ImGui::CloseCurrentPopup();
        }
        ImGui::PopStyleColor(3);
        
        ImGui::EndPopup();
    }
    else {
        openNewProject = false; // reset flag if user clicked X or background on modal
    }
    
    ImGui::PopStyleColor(2);
    ImGui::PopStyleVar(2);

    ImGui::End();
    ImGui::PopStyleColor(); // Pop WindowBg color
    ImGui::PopStyleVar();
}
