#pragma once
#include "Component.h"
#include "GameObject.h"
#include <SDL3/SDL.h>
#include <cmath>

class PlayerController : public Component
{
public:
    void Update(float deltaTime) override
    {
        if (!owner) return;

        const bool* keyboard = SDL_GetKeyboardState(NULL);
        float accel = 800.0f;

        // Platformer Jump (Setting velocity directly to create the upward impulse)
        if (keyboard[SDL_SCANCODE_SPACE] && owner->isGrounded) 
        {
            owner->velocity.y = -800.0f; // Jump force
        }

        // Horizontal movement (Lerp / Friction)
        float targetVelocityX = 0.0f;
        float moveSpeed = 400.0f;
        
        if (keyboard[SDL_SCANCODE_A]) 
        {
            targetVelocityX = -moveSpeed;
            owner->flipHorizontal = true;
        }
        if (keyboard[SDL_SCANCODE_D]) 
        {
            targetVelocityX = moveSpeed;
            owner->flipHorizontal = false;
        }

        // Apply Lerp for smooth acceleration and deceleration
        float lerpFactor = 10.0f * deltaTime;
        owner->velocity.x += (targetVelocityX - owner->velocity.x) * lerpFactor;

        // Animation States (Direct State setting for row-based sheet)
        if (!owner->isGrounded)
        {
            owner->state = AnimationState::Jump;
        }
        else if (std::abs(owner->velocity.x) > 20.0f) // Threshold to prevent sliding pixel jitter
        {
            owner->state = AnimationState::Walk;
        }
        else
        {
            owner->state = AnimationState::Idle;
        }
    }
};
