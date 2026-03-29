#pragma once
#include "Component.h"
#include "GameObject.h"
#include "CombatComponent.h"
#include "HealthComponent.h"
#include <SDL3/SDL.h>
#include <cmath>
#include <string>

// ─────────────────────────────────────────────────────────────────────────────
//  PlayerController
//  Specifically manages player input (A/D/Space/J) and animation states.
// ─────────────────────────────────────────────────────────────────────────────
class PlayerController : public Component
{
public:
    std::string GetName() const override { return "PlayerController"; }

    void Update(float deltaTime) override
    {
        if (!owner) return;

        const bool* keyboard = SDL_GetKeyboardState(NULL);
        auto* combat = owner->GetComponent<CombatComponent>();

        // ── J key — trigger attack (handled by CombatComponent logic) ─────────
        if (keyboard[SDL_SCANCODE_J] && combat)
            combat->Attack();

        // ── Platformer Jump ───────────────────────────────────────────────────
        if (keyboard[SDL_SCANCODE_SPACE] && owner->isGrounded)
            owner->velocity.y = -800.0f;

        // ── Horizontal movement ───────────────────────────────────────────────
        float targetVelocityX = 0.0f;
        float moveSpeed = 400.0f;

        if (keyboard[SDL_SCANCODE_A])
        {
            targetVelocityX      = -moveSpeed;
            owner->flipHorizontal = true;
        }
        if (keyboard[SDL_SCANCODE_D])
        {
            targetVelocityX      = moveSpeed;
            owner->flipHorizontal = false;
        }

        float lerpFactor = 10.0f * deltaTime;
        owner->velocity.x += (targetVelocityX - owner->velocity.x) * lerpFactor;

        // ── Animation state ───────────────────────────────────────────────────
        if (!owner->isGrounded)
            owner->state = AnimationState::Jump;
        else if (std::abs(owner->velocity.x) > 20.0f)
            owner->state = AnimationState::Walk;
        else
            owner->state = AnimationState::Idle;
    }
};
