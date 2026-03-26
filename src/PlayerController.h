#pragma once
#include "Component.h"
#include "GameObject.h"
#include "AnimatorComponent.h"
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

        // Horizontal movement
        if (keyboard[SDL_SCANCODE_A]) owner->velocity.x -= accel * deltaTime;
        if (keyboard[SDL_SCANCODE_D]) owner->velocity.x += accel * deltaTime;

        // Animation States (The Engine's first brain layer!)
        AnimatorComponent* anim = owner->GetComponent<AnimatorComponent>();
        if (anim)
        {
            if (!owner->isGrounded)
            {
                anim->Play("Jump");
            }
            else if (std::abs(owner->velocity.x) > 20.0f) // Threshold to prevent sliding pixel jitter
            {
                anim->Play("Run");
            }
            else
            {
                anim->Play("Idle");
            }
        }
    }
};
