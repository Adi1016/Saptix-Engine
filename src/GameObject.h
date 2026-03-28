#pragma once
#include <vector>
#include <string>
#include <SDL3/SDL.h>
#include "Vector2.h"
#include "Component.h"

enum class AnimationState
{
    Idle,
    Walk,
    Jump
};

struct GameObject
{
    std::string name = "GameObject";
    Vector2 position;
    Vector2 size;
    Vector2 velocity;
    bool flipHorizontal = false;
    bool isGrounded = false;
    
    // Animation tracking
    int currentFrame = 0;
    float animationTimer = 0.0f;
    float animationSpeed = 0.1f;

    // Sprite Sheet data
    std::vector<SDL_FRect> framesIdle;
    std::vector<SDL_FRect> framesWalk;
    std::vector<SDL_FRect> framesJump;
    
    AnimationState state = AnimationState::Idle;

    SDL_Texture* texture = nullptr;

    std::vector<Component*> components;

    template <typename T>
    T* GetComponent()
    {
        for (auto comp : components)
        {
            T* target = dynamic_cast<T*>(comp);
            if (target) return target;
        }
        return nullptr;
    }

    void Update(float deltaTime)
    {
        // update all components
        for (auto comp : components)
        {
            comp->Update(deltaTime);
        }
    }
};