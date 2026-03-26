#pragma once
#include "Component.h"
#include "GameObject.h"
#include <string>
#include <map>
#include <fstream>
#include <iostream>
#include <sstream>

struct AnimationState
{
    int startFrame;
    int endFrame;
    float speed;
};

class AnimatorComponent : public Component
{
public:
    std::map<std::string, AnimationState> animations;
    std::string currentAnim = "";

    void LoadAnimFile(const std::string& path)
    {
        std::ifstream file(path);
        if (!file.is_open())
        {
            std::string errMsg = "Failed to open anim file: " + path + "\nPlease make sure player.sanim is in the correct working directory!";
            SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Animator Error", errMsg.c_str(), NULL);
            return;
        }

        std::string line;
        while (std::getline(file, line))
        {
            std::stringstream ss(line);
            std::string type;
            ss >> type;

            if (type == "frame_width")
                ss >> owner->frameWidth;
            else if (type == "frame_height")
                ss >> owner->frameHeight;
            else if (type == "anim")
            {
                std::string name;
                int start, end;
                float speed;
                // Parse: anim [Name] [Start] [End] [Speed]
                ss >> name >> start >> end >> speed;
                animations[name] = { start, end, speed };
            }
        }
    }

    void Play(const std::string& animName)
    {
        if (currentAnim == animName) return; // already playing this state
        if (animations.find(animName) == animations.end()) return; // doesn't exist

        currentAnim = animName;
        auto& anim = animations[animName];

        owner->startFrame = anim.startFrame;
        owner->endFrame = anim.endFrame;
        owner->animationSpeed = anim.speed;
        
        // Snap immediately to the new animation starting frame
        owner->currentFrame = anim.startFrame;
        owner->animationTimer = 0.0f;
    }

    void Update(float deltaTime) override
    {
        // the animator component doesn't need to manually tick frames,
        // because Scene.h handles time using GameObject's startFrame/endFrame fields natively!
    }
};
