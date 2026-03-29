#pragma once
#include "Component.h"
#include "GameObject.h"
#include <SDL3/SDL.h>
#include <cmath>
#include <vector>

// ─────────────────────────────────────────────────────
//  Enemy Behaviour Modes
// ─────────────────────────────────────────────────────
enum class EnemyMode
{
    Patrol,   // Walks back and forth between two X waypoints
    Chase,    // Moves directly toward the player when in detection range
    Idle      // Stands still (stunned, dead, etc.)
};

class EnemyController : public Component
{
public:
    std::string GetName() const override { return "EnemyController"; }

    // ── Tuneable parameters ──────────────────────────
    float   patrolSpeed     = 120.0f;   // px/s while patrolling
    float   chaseSpeed      = 220.0f;   // px/s while chasing
    float   detectionRange  = 320.0f;   // px — aggro radius
    float   patrolRangeX    = 200.0f;   // px left/right from spawn
    EnemyMode mode          = EnemyMode::Patrol;

    // ── Internal state ───────────────────────────────
    float   spawnX          = 0.0f;     // set on first update
    bool    spawnSet        = false;
    float   patrolDir       = 1.0f;     // +1 right / -1 left

    // Pointer to the scene's object list — set by Scene::Update each frame
    std::vector<GameObject>* sceneObjects = nullptr;

    void Update(float deltaTime) override
    {
        if (!owner) return;

        // Initialise spawn point once
        if (!spawnSet)
        {
            spawnX = owner->position.x;
            spawnSet = true;
        }

        // ── Find the first non-enemy object as the player ──
        GameObject* player = nullptr;
        if (sceneObjects)
        {
            for (auto& obj : *sceneObjects)
            {
                if (!obj.isEnemy)
                {
                    player = &obj;
                    break;
                }
            }
        }

        // ── State transitions ───────────────────────────────
        if (player)
        {
            float dx = player->position.x - owner->position.x;
            float dy = player->position.y - owner->position.y;
            float dist = std::sqrt(dx * dx + dy * dy);

            if (dist <= detectionRange)
                mode = EnemyMode::Chase;
            else
                mode = EnemyMode::Patrol;
        }
        else
        {
            mode = EnemyMode::Patrol;
        }

        // ── Behaviour execution ─────────────────────────────
        switch (mode)
        {
            case EnemyMode::Chase:
            {
                if (!player) break;
                float dx = player->position.x - owner->position.x;
                float dir = (dx > 0) ? 1.0f : -1.0f;

                owner->velocity.x = dir * chaseSpeed;
                owner->flipHorizontal = (dir < 0);

                // Animation state
                owner->state = AnimationState::Walk;
                break;
            }

            case EnemyMode::Patrol:
            {
                // Reverse at patrol boundary
                float leftBound  = spawnX - patrolRangeX;
                float rightBound = spawnX + patrolRangeX;

                if (owner->position.x <= leftBound)  patrolDir =  1.0f;
                if (owner->position.x >= rightBound) patrolDir = -1.0f;

                owner->velocity.x = patrolDir * patrolSpeed;
                owner->flipHorizontal = (patrolDir < 0);
                owner->state = AnimationState::Walk;
                break;
            }

            case EnemyMode::Idle:
            {
                owner->velocity.x = 0;
                owner->state = AnimationState::Idle;
                break;
            }
        }
    }
};
