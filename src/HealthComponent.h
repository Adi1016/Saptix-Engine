#pragma once
#include "Component.h"
#include "GameObject.h"
#include "Vector2.h"
#include <SDL3/SDL.h>
#include <algorithm>
#include <string>

// ─────────────────────────────────────────────────────────────────────────────
//  HealthComponent
//  A component that manages health, damage iframes, and draws a health bar.
//  New Version: Owns its own health state directly.
// ─────────────────────────────────────────────────────────────────────────────
class HealthComponent : public Component
{
public:
    std::string GetName() const override { return "HealthComponent"; }

    // ── Health State ────────────────────────────────────────────────────────
    int   maxHealth          = 100;
    int   currentHealth      = 100;
    bool  isAlive            = true;
    float invincibilityTimer = 0.0f;  // Seconds of iframe left after a hit
    float damageFlashTimer   = 0.0f;  // Brief tinting on hit

    // ── Settings (inspector-exposed) ────────────────────────────────────────
    bool  showBar           = true;     
    float barWidth          = 60.0f;    
    float barHeight         = 8.0f;     
    float barOffsetY        = -16.0f;   

    // Bar colors
    SDL_Color colHigh   = {117, 255, 158, 255}; 
    SDL_Color colMid    = {255, 200, 60,  255}; 
    SDL_Color colLow    = {255, 60,  60,  255}; 
    SDL_Color colFlash  = {255, 255, 255, 255}; 
    SDL_Color colTrack  = {30,  30,  30,  210}; 

    float iframeDuration = 0.8f;    
    float flashDuration  = 0.2f;    

    // ── Methods ─────────────────────────────────────────────────────────────
    void TakeDamage(int amount)
    {
        if (invincibilityTimer > 0.0f || !isAlive) return;

        currentHealth -= amount;
        invincibilityTimer = iframeDuration;
        damageFlashTimer   = flashDuration;

        if (currentHealth <= 0)
        {
            currentHealth = 0;
            isAlive = false;
        }
    }

    void Heal(int amount)
    {
        if (!isAlive) return;
        currentHealth = std::min(currentHealth + amount, maxHealth);
    }

    void ResetHealth()
    {
        currentHealth      = maxHealth;
        isAlive            = true;
        invincibilityTimer = 0.0f;
        damageFlashTimer   = 0.0f;
    }

    void Update(float deltaTime) override
    {
        if (invincibilityTimer > 0.0f) invincibilityTimer -= deltaTime;
        if (damageFlashTimer   > 0.0f) damageFlashTimer   -= deltaTime;
    }

    void RenderBar(SDL_Renderer* renderer, const Vector2& cameraPos) const
    {
        if (!owner || !showBar || maxHealth <= 0) return;

        float pct = std::max(0.0f, std::min(1.0f, (float)currentHealth / (float)maxHealth));

        float worldX = owner->position.x + owner->size.x * 0.5f - barWidth * 0.5f;
        float worldY = owner->position.y + barOffsetY - barHeight;

        float sx = worldX - cameraPos.x;
        float sy = worldY - cameraPos.y;

        SDL_FRect track = {sx, sy, barWidth, barHeight};
        SDL_SetRenderDrawColor(renderer, colTrack.r, colTrack.g, colTrack.b, colTrack.a);
        SDL_RenderFillRect(renderer, &track);

        SDL_Color fill;
        if (damageFlashTimer > 0.0f)
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

        SDL_SetRenderDrawColor(renderer, 60, 60, 60, 180);
        SDL_RenderRect(renderer, &track);
    }
};
