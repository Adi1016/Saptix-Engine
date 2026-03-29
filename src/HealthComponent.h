#pragma once
#include "Component.h"
#include "GameObject.h"
#include <SDL3/SDL.h>
#include <algorithm>

// ─────────────────────────────────────────────────────────────────────────────
//  HealthComponent
//  A component that manages health, damage iframes, and draws a health bar
//  directly into the game world (SDL render target) — NOT the editor overlay.
//  Can be attached to ANY GameObject: player, enemy, boss, destructible prop.
// ─────────────────────────────────────────────────────────────────────────────
class HealthComponent : public Component
{
public:
    std::string GetName() const override { return "HealthComponent"; }

    // ── Settings (all exposed in the Properties panel) ───────────────────────
    bool  showBar           = true;      // Draw world-space HP bar above entity
    float barWidth          = 60.0f;     // Bar width in world pixels
    float barHeight         = 8.0f;      // Bar height in world pixels
    float barOffsetY        = -16.0f;    // Vertical offset above the entity top

    // Bar fill colours (RGBA bytes)
    SDL_Color colHigh   = {117, 255, 158, 255};  // >60% — neon green
    SDL_Color colMid    = {255, 200, 60,  255};  // 30-60% — amber
    SDL_Color colLow    = {255, 60,  60,  255};  // <30% — red
    SDL_Color colFlash  = {255, 255, 255, 255};  // Hit flash — white
    SDL_Color colTrack  = {30,  30,  30,  210};  // Background track

    float iframeDuration = 0.8f;    // Seconds of invincibility after a hit
    float flashDuration  = 0.2f;    // Seconds of hit-flash

    // ── Runtime helpers ─────────────────────────────────────────────────────
    void TakeDamage(int amount)
    {
        if (!owner || owner->invincibilityTimer > 0.0f || !owner->isAlive) return;

        owner->health            -= amount;
        owner->invincibilityTimer = iframeDuration;
        owner->damageFlashTimer   = flashDuration;

        if (owner->health <= 0)
        {
            owner->health  = 0;
            owner->isAlive = false;
        }
    }

    void Heal(int amount)
    {
        if (!owner || !owner->isAlive) return;
        owner->health = std::min(owner->health + amount, owner->maxHealth);
    }

    void ResetHealth()
    {
        if (!owner) return;
        owner->health             = owner->maxHealth;
        owner->isAlive            = true;
        owner->invincibilityTimer = 0.0f;
        owner->damageFlashTimer   = 0.0f;
    }

    // ── Called by Scene::Render — draws into the world (offscreen texture) ──
    void RenderBar(SDL_Renderer* renderer, const Vector2& cameraPos) const
    {
        if (!owner || !showBar || owner->maxHealth <= 0) return;

        float pct = std::max(0.0f, std::min(1.0f,
            (float)owner->health / (float)owner->maxHealth));

        // Centre the bar above the entity
        float worldX = owner->position.x + owner->size.x * 0.5f - barWidth * 0.5f;
        float worldY = owner->position.y + barOffsetY - barHeight;

        float sx = worldX - cameraPos.x;
        float sy = worldY - cameraPos.y;

        // Track
        SDL_FRect track = {sx, sy, barWidth, barHeight};
        SDL_SetRenderDrawColor(renderer, colTrack.r, colTrack.g, colTrack.b, colTrack.a);
        SDL_RenderFillRect(renderer, &track);

        // Fill with hit-flash or gradient colour
        SDL_Color fill;
        if (owner->damageFlashTimer > 0.0f)
            fill = colFlash;
        else if (pct > 0.6f)
            fill = colHigh;
        else if (pct > 0.3f)
            fill = colMid;
        else
            fill = colLow;

        SDL_FRect bar = {sx, sy, barWidth * pct, barHeight};
        SDL_SetRenderDrawColor(renderer, fill.r, fill.g, fill.b, fill.a);
        SDL_RenderFillRect(renderer, &bar);

        // Thin border
        SDL_SetRenderDrawColor(renderer, 60, 60, 60, 180);
        SDL_RenderRect(renderer, &track);
    }

    void Update(float deltaTime) override
    {
        // Timers are ticked by Scene.h for performance — nothing to do here
    }
};
