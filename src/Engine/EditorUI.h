#pragma once
#include <SDL3/SDL.h>
#include "imgui.h"
#include "imgui_impl_sdl3.h"
#include "imgui_impl_sdlrenderer3.h"
#include "../Scene.h"
#include "../Serializer.h"
#include <string>
#include <vector>
#include <algorithm>

class EditorUI
{
public:
    GameObject* selectedObject = nullptr;
    
private:
    // Window toggles
    bool showSpriteEditor = false;

    // Rename state
    bool renamingActive = false;
    char renameBuffer[128] = {};

    // Sprite Slicer state
    std::vector<SDL_FRect> slicedFrames;
    std::vector<int> selectedSlices;
    ImVec2 selectionStart;
    bool isSelecting = false;
    
    // Grid-slice helper
    int sliceW = 64, sliceH = 64;

    // Animation preview timer
    int previewState = 0;
    float previewTimer = 0.0f;
    int previewFrame = 0;

public:
    void RenderUI(Scene& scene, SDL_Renderer* renderer, SDL_Texture* gameViewportTex,
                  SDL_Texture* spriteTex, int spriteW, int spriteH)
    {
        SetupDockspace();
        DrawMainMenuBar(scene, spriteTex);
        DrawHierarchy(scene);
        DrawGameViewport(gameViewportTex);
        DrawProperties(scene);
        
        if (showSpriteEditor && selectedObject && selectedObject->texture)
        {
            DrawSpriteSheetEditor(spriteTex, spriteW, spriteH);
        }
    }

private:
    void SetupDockspace()
    {
        ImGuiViewport* vp = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(vp->WorkPos);
        ImGui::SetNextWindowSize(vp->WorkSize);
        ImGui::SetNextWindowViewport(vp->ID);

        ImGuiWindowFlags host_flags =
            ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoTitleBar |
            ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus |
            ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_NoBackground;

        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
        ImGui::Begin("##DockHost", nullptr, host_flags);
        ImGui::PopStyleVar(3);

        ImGuiID dockId = ImGui::GetID("SaptixDock");
        ImGui::DockSpace(dockId, ImVec2(0,0), ImGuiDockNodeFlags_PassthruCentralNode);
        ImGui::End();
    }

    void DrawMainMenuBar(Scene& scene, SDL_Texture* spriteTex)
    {
        if (ImGui::BeginMainMenuBar())
        {
            if (ImGui::BeginMenu("File"))
            {
                if (ImGui::MenuItem("New Scene"))     scene.objects.clear();
                if (ImGui::MenuItem("Save Scene"))    Serializer::SaveScene(scene, "myscene.saptix");
                if (ImGui::MenuItem("Load Scene"))    Serializer::LoadScene(scene, "myscene.saptix", spriteTex);
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("GameObject"))
            {
                if (ImGui::MenuItem("Create Empty"))
                {
                    scene.objects.push_back({ "New Object", {200,200},{50,50},{0,0} });
                }
                if (ImGui::MenuItem("Delete Selected") && selectedObject)
                {
                    scene.objects.erase(
                        std::remove_if(scene.objects.begin(), scene.objects.end(),
                            [&](const GameObject& o){ return &o == selectedObject; }),
                        scene.objects.end());
                    selectedObject = nullptr;
                }
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Window"))
            {
                ImGui::MenuItem("Sprite Editor", nullptr, &showSpriteEditor);
                ImGui::EndMenu();
            }

            ImGuiIO& io = ImGui::GetIO();
            ImGui::SetCursorPosX(io.DisplaySize.x - 240);
            ImGui::TextColored(ImVec4(0.3f, 1.0f, 0.5f, 1.0f), "▶  SIMULATING   [F5 = hide editor]");

            ImGui::EndMainMenuBar();
        }
    }

    void DrawHierarchy(Scene& scene)
    {
        ImGui::SetNextWindowSize(ImVec2(240, 500), ImGuiCond_FirstUseEver);
        ImGui::Begin("Hierarchy");

        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.15f, 0.55f, 0.25f, 1.0f));
        if (ImGui::Button("+ Add Object", ImVec2(-1, 0)))
        {
            scene.objects.push_back({ "New Object", {200,200},{50,50},{0,0} });
        }
        ImGui::PopStyleColor();

        ImGui::Separator();
        ImGui::TextDisabled("Scene Objects (%d)", (int)scene.objects.size());
        ImGui::Separator();

        for (int i = 0; i < (int)scene.objects.size(); i++)
        {
            GameObject& obj = scene.objects[i];
            bool isSelected = (selectedObject == &obj);

            if (renamingActive && isSelected)
            {
                ImGui::SetNextItemWidth(-1);
                if (ImGui::InputText("##rename", renameBuffer, sizeof(renameBuffer),
                                     ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll))
                {
                    obj.name = renameBuffer;
                    renamingActive = false;
                }
                if (!ImGui::IsItemActive() && !ImGui::IsItemFocused())
                {
                    obj.name = renameBuffer;
                    renamingActive = false;
                }
            }
            else
            {
                if (ImGui::Selectable((obj.name + "##" + std::to_string(i)).c_str(), isSelected, ImGuiSelectableFlags_AllowDoubleClick))
                {
                    selectedObject = &obj;
                    if (ImGui::IsMouseDoubleClicked(0))
                    {
                        renamingActive = true;
                        strncpy_s(renameBuffer, obj.name.c_str(), sizeof(renameBuffer) - 1);
                        ImGui::SetKeyboardFocusHere(-1);
                    }
                }

                if (ImGui::BeginPopupContextItem(("ctx##" + std::to_string(i)).c_str()))
                {
                    selectedObject = &obj;
                    if (ImGui::MenuItem("Rename"))
                    {
                        renamingActive = true;
                        strncpy_s(renameBuffer, obj.name.c_str(), sizeof(renameBuffer) - 1);
                    }
                    if (ImGui::MenuItem("Duplicate"))
                    {
                        GameObject copy = obj;
                        copy.name += " (Copy)";
                        copy.position.x += 20; copy.position.y += 20;
                        scene.objects.insert(scene.objects.begin() + i + 1, copy);
                    }
                    ImGui::Separator();
                    if (ImGui::MenuItem("Delete"))
                    {
                        scene.objects.erase(scene.objects.begin() + i);
                        if (selectedObject == &obj) selectedObject = nullptr;
                        ImGui::EndPopup();
                        break;
                    }
                    ImGui::EndPopup();
                }
            }
        }

        if (selectedObject && ImGui::IsWindowFocused() && ImGui::IsKeyPressed(ImGuiKey_Delete))
        {
            scene.objects.erase(
                std::remove_if(scene.objects.begin(), scene.objects.end(),
                    [&](const GameObject& o){ return &o == selectedObject; }),
                scene.objects.end());
            selectedObject = nullptr;
        }

        ImGui::End();
    }

    void DrawGameViewport(SDL_Texture* gameViewportTex)
    {
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(2, 2));
        ImGui::SetNextWindowSize(ImVec2(760, 480), ImGuiCond_FirstUseEver);
        ImGui::Begin("Game");

        ImVec2 avail = ImGui::GetContentRegionAvail();
        float aspect = 1280.0f / 720.0f;
        float drawW  = avail.x;
        float drawH  = drawW / aspect;
        if (drawH > avail.y) { drawH = avail.y; drawW = drawH * aspect; }

        ImVec2 cursor = ImGui::GetCursorScreenPos();
        float  offX   = cursor.x + (avail.x - drawW) * 0.5f;
        float  offY   = cursor.y + (avail.y - drawH) * 0.5f;

        ImGui::GetWindowDrawList()->AddImage(ImTextureRef{gameViewportTex}, ImVec2(offX, offY), ImVec2(offX + drawW, offY + drawH));

        ImGui::End();
        ImGui::PopStyleVar();
    }

    void DrawProperties(Scene& scene)
    {
        ImGui::SetNextWindowSize(ImVec2(280, 500), ImGuiCond_FirstUseEver);
        ImGui::Begin("Properties");

        if (!selectedObject)
        {
            ImGui::TextDisabled("No object selected.");
            ImGui::End();
            return;
        }

        char nameBuf[128];
        strncpy_s(nameBuf, selectedObject->name.c_str(), sizeof(nameBuf) - 1);
        ImGui::SetNextItemWidth(-1);
        if (ImGui::InputText("##name", nameBuf, sizeof(nameBuf)))
            selectedObject->name = nameBuf;

        ImGui::Separator();

        if (ImGui::BeginTabBar("PropsTabBar"))
        {
            if (ImGui::BeginTabItem("Transform"))
            {
                ImGui::Text("Position");
                ImGui::DragFloat2("##pos", &selectedObject->position.x, 1.0f);
                ImGui::Text("Size");
                ImGui::DragFloat2("##size", &selectedObject->size.x, 1.0f);
                ImGui::Text("Velocity");
                ImGui::DragFloat2("##vel", &selectedObject->velocity.x, 0.5f);
                ImGui::Separator();
                ImGui::Checkbox("Is Grounded", &selectedObject->isGrounded);
                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("Animation"))
            {
                if (selectedObject->texture)
                {
                    if (ImGui::Button("Open Sprite Sheet Editor", ImVec2(-1, 30)))
                    {
                        showSpriteEditor = true;
                    }
                }
                else
                {
                    ImGui::TextDisabled("Assign a texture in Engine.cpp");
                }

                ImGui::Separator();
                ImGui::DragFloat("Anim Speed", &selectedObject->animationSpeed, 0.01f, 0.01f, 2.0f);
                
                int stateInt = (int)selectedObject->state;
                const char* states[] = { "Idle", "Walk", "Jump" };
                if (ImGui::Combo("State##combo", &stateInt, states, 3))
                    selectedObject->state = (AnimationState)stateInt;

                // Frame info
                const std::vector<SDL_FRect>* activeAnim = nullptr;
                switch (selectedObject->state) {
                    case AnimationState::Idle: activeAnim = &selectedObject->framesIdle; break;
                    case AnimationState::Walk: activeAnim = &selectedObject->framesWalk; break;
                    case AnimationState::Jump: activeAnim = &selectedObject->framesJump; break;
                }
                
                if (activeAnim && !activeAnim->empty())
                    ImGui::Text("Current Frame: %d / %d", selectedObject->currentFrame, (int)activeAnim->size() - 1);
                else
                    ImGui::Text("No frames mapped to this state.");

                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("Physics"))
            {
                ImGui::DragFloat("Gravity Scale", &scene.gravity, 0.1f, 0.0f, 5000.0f);
                ImGui::EndTabItem();
            }

            ImGui::EndTabBar();
        }

        ImGui::End();
    }

    void DrawSpriteSheetEditor(SDL_Texture* spriteTexture, int spriteTexWidth, int spriteTexHeight)
    {
        ImGui::SetNextWindowSize(ImVec2(900, 600), ImGuiCond_FirstUseEver);
        ImGui::Begin("Sprite Sheet Editor", &showSpriteEditor);

        float canvasW = (float)(spriteTexWidth > 0 ? spriteTexWidth : 256);
        float canvasH = (float)(spriteTexHeight > 0 ? spriteTexHeight : 256);

        // 3 Column Layout
        // [ Canvas ] [ States List ] [ Preview ]
        ImGui::Columns(3, "SpriteEdCols", false);
        ImGui::SetColumnWidth(0, canvasW + 40.0f > 500.0f ? 500.0f : canvasW + 40.0f);
        ImGui::SetColumnWidth(1, 250.0f);

        // ==========================================
        // COLUMN 1: Canvas & Slicing
        // ==========================================
        ImGui::Text("1. Slice Frames");
        ImGui::Separator();
        ImGui::SetNextItemWidth(60); ImGui::InputInt("W##gs", &sliceW, 0); ImGui::SameLine();
        ImGui::SetNextItemWidth(60); ImGui::InputInt("H##gs", &sliceH, 0); ImGui::SameLine();
        if (ImGui::Button("Auto-Slice Grid"))
        {
            slicedFrames.clear();
            int cols = (spriteTexWidth + sliceW - 1) / sliceW;  // Round up to grab any trailing pixels
            int rows = (spriteTexHeight + sliceH - 1) / sliceH;
            for (int r = 0; r < rows; r++)
            for (int c = 0; c < cols; c++)
            {
                slicedFrames.push_back({ (float)(c * sliceW), (float)(r * sliceH), (float)sliceW, (float)sliceH });
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("Clear")) { slicedFrames.clear(); selectedSlices.clear(); }

        ImVec2 canvasOrigin = ImGui::GetCursorScreenPos();
        float dispScale = 1.0f;
        if (canvasW > 460.0f) dispScale = 460.0f / canvasW;
        float dispW = canvasW * dispScale;
        float dispH = canvasH * dispScale;

        ImGui::Image(ImTextureRef{spriteTexture}, ImVec2(dispW, dispH));

        ImDrawList* dl = ImGui::GetWindowDrawList();
        for (int i = 0; i < (int)slicedFrames.size(); i++)
        {
            SDL_FRect& fr = slicedFrames[i];
            ImVec2 tl = { canvasOrigin.x + fr.x * dispScale, canvasOrigin.y + fr.y * dispScale };
            ImVec2 br = { tl.x + fr.w * dispScale, tl.y + fr.h * dispScale };
            bool isSel = (std::find(selectedSlices.begin(), selectedSlices.end(), i) != selectedSlices.end());
            ImU32 col = isSel ? IM_COL32(80,255,100,255) : IM_COL32(200,200,200,100);
            dl->AddRect(tl, br, col, 0, 0, isSel ? 2.0f : 1.0f);
        }

        // Invisible button over canvas to catch click and drag events
        ImGui::SetCursorScreenPos(canvasOrigin);
        ImGui::InvisibleButton("CanvasHitbox", ImVec2(dispW, dispH));

        if (ImGui::IsItemActivated())
        {
            if (!ImGui::GetIO().KeyShift) selectedSlices.clear();
            selectionStart = ImGui::GetMousePos();
            isSelecting = true;
        }

        if (ImGui::IsItemActive() && ImGui::IsMouseDragging(0))
        {
            ImVec2 mpos = ImGui::GetMousePos();
            dl->AddRect(selectionStart, mpos, IM_COL32(255, 255, 0, 255));
        }

        if (isSelecting && ImGui::IsMouseReleased(0))
        {
            isSelecting = false;
            ImVec2 mpos = ImGui::GetMousePos();
            float minX = (std::min(selectionStart.x, mpos.x) - canvasOrigin.x) / dispScale;
            float maxX = (std::max(selectionStart.x, mpos.x) - canvasOrigin.x) / dispScale;
            float minY = (std::min(selectionStart.y, mpos.y) - canvasOrigin.y) / dispScale;
            float maxY = (std::max(selectionStart.y, mpos.y) - canvasOrigin.y) / dispScale;

            if (std::abs(maxX - minX) < 1.0f && std::abs(maxY - minY) < 1.0f)
            {
                maxX = minX + 1.0f;
                maxY = minY + 1.0f;
            }

            for (int i = 0; i < (int)slicedFrames.size(); i++)
            {
                SDL_FRect& fr = slicedFrames[i];
                bool overlap = !(fr.x + fr.w < minX || fr.x > maxX || fr.y + fr.h < minY || fr.y > maxY);
                if (overlap) {
                    if (std::find(selectedSlices.begin(), selectedSlices.end(), i) == selectedSlices.end()) {
                        selectedSlices.push_back(i);
                    }
                }
            }
            std::sort(selectedSlices.begin(), selectedSlices.end());
        }

        ImGui::NextColumn();

        // ==========================================
        // COLUMN 2: Animation States Map
        // ==========================================
        ImGui::Text("2. Map to States");
        ImGui::Separator();
        
        if (!selectedSlices.empty())
        {
            ImGui::TextColored(ImVec4(0.3f, 1.0f, 0.3f, 1.0f), "[%d Slices Selected]", (int)selectedSlices.size());
            
            auto AddAllTo = [&](std::vector<SDL_FRect>& dest) {
                for (int idx : selectedSlices) dest.push_back(slicedFrames[idx]);
            };

            if (ImGui::Button("Add ALL to IDLE state", ImVec2(-1, 0))) AddAllTo(selectedObject->framesIdle);
            if (ImGui::Button("Add ALL to WALK state", ImVec2(-1, 0))) AddAllTo(selectedObject->framesWalk);
            if (ImGui::Button("Add ALL to JUMP state", ImVec2(-1, 0))) AddAllTo(selectedObject->framesJump);
        }
        else
        {
            ImGui::TextDisabled("Select a slice on the left...");
            ImGui::Dummy(ImVec2(0, 75)); // padding
        }

        ImGui::Dummy(ImVec2(0, 10));
        
        auto DrawStateList = [&](const char* label, std::vector<SDL_FRect>& list) {
            ImGui::Text("%s (%d frames)", label, (int)list.size());
            ImGui::BeginChild(label, ImVec2(0, 100), true);
            for (int i = 0; i < (int)list.size(); i++)
            {
                ImGui::PushID(i);
                ImGui::Text("Fr%d: [%.0f, %.0f]", i, list[i].x, list[i].y);
                ImGui::SameLine(180);
                if (ImGui::Button("X")) { list.erase(list.begin() + i); ImGui::PopID(); break; }
                ImGui::PopID();
            }
            ImGui::EndChild();
            if (ImGui::Button((std::string("Clear ") + label).c_str(), ImVec2(-1, 0))) list.clear();
            ImGui::Dummy(ImVec2(0, 5));
        };

        DrawStateList("IDLE", selectedObject->framesIdle);
        DrawStateList("WALK", selectedObject->framesWalk);
        DrawStateList("JUMP", selectedObject->framesJump);

        ImGui::NextColumn();

        // ==========================================
        // COLUMN 3: Live Preview
        // ==========================================
        ImGui::Text("3. Live Preview");
        ImGui::Separator();
        
        const char* states[] = { "Idle", "Walk", "Jump" };
        ImGui::Combo("Previewing State", &previewState, states, 3);
            
        const std::vector<SDL_FRect>* activeAnim = nullptr;
        switch (previewState) {
            case 0: activeAnim = &selectedObject->framesIdle; break;
            case 1: activeAnim = &selectedObject->framesWalk; break;
            case 2: activeAnim = &selectedObject->framesJump; break;
        }

        if (activeAnim && !activeAnim->empty())
        {
            // Advance embedded preview timer
            previewTimer += ImGui::GetIO().DeltaTime;
            if (previewTimer >= selectedObject->animationSpeed)
            {
                previewTimer = 0.0f;
                previewFrame++;
            }
            
            // Unconditional bounds check avoids vector out-of-range crash if activeAnim size shrinks!
            if (previewFrame >= activeAnim->size()) previewFrame = 0;

            SDL_FRect fr = (*activeAnim)[previewFrame];
            
            // Map pixel coordinates to UV coordinates (0.0 to 1.0)
            ImVec2 uv0 = ImVec2(fr.x / spriteTexWidth, fr.y / spriteTexHeight);
            ImVec2 uv1 = ImVec2((fr.x + fr.w) / spriteTexWidth, (fr.y + fr.h) / spriteTexHeight);
            
            // Draw huge preview
            ImGui::Dummy(ImVec2(0, 20));
            ImGui::Image(ImTextureRef{spriteTexture}, ImVec2(200, 200), uv0, uv1);
            ImGui::Text("Playing Frame %d", previewFrame);
        }
        else
        {
            ImGui::Dummy(ImVec2(0, 20));
            ImGui::TextDisabled("No frames mapped to this state.");
        }

        ImGui::Columns(1);
        ImGui::End();
    }
};
