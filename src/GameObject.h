#pragma once
#include <vector>
#include <SDL3/SDL.h>
#include "Vector2.h"
#include "Component.h"

struct GameObject
{
    Vector2 position;
    Vector2 size;
    Vector2 velocity;
    bool isGrounded = false;
    
    // Animation tracking
    int currentFrame = 0;
    float animationTimer = 0.0f;
    float animationSpeed = 0.1f;

    // Sprite Sheet data
    int frameWidth = 64;
    int frameHeight = 64;
    int startFrame = 0;
    int endFrame = 0;

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