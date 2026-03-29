#pragma once
#include <vector>
#include <algorithm>
#include <SDL3/SDL.h>
#include "GameObject.h"
#include "TileMap.h"
#include "EnemyController.h"
#include "HealthComponent.h"
#include "CombatComponent.h"
#include "Camera.h"
#include "PlayerController.h"

class Scene
{
public:
    std::vector<GameObject> objects;
    TileMap tileMap;

    int screenWidth  = 800;
    int screenHeight = 600;

    int worldWidth   = 2000;
    int worldHeight  = 2000;

    float gravity = 1500.0f; // Public so editor can tweak it live

    // Utility to find the first object with a specific component type
    template <typename T>
    GameObject* FindObjectWithComponent()
    {
        for (auto& obj : objects)
        {
            if (obj.GetComponent<T>()) return &obj;
        }
        return nullptr;
    }

    template <typename T>
    const GameObject* FindObjectWithComponent() const
    {
        for (const auto& obj : objects)
        {
            if (obj.GetComponent<T>()) return &obj;
        }
        return nullptr;
    }

    void Update(float deltaTime, Camera& camera)
    {
        // Vector reallocation safety map: Guarantee all components point to their true memory address every frame
        for (auto& obj : objects)
        {
            for (auto* comp : obj.components)
            {
                comp->owner = &obj;
            }
        }

        // Move all objects + resolve collisions using split-axis logic
        for (auto& obj : objects)
        {
            auto* health = obj.GetComponent<HealthComponent>();

            // ── Dead Unit: freeze and skip all further physics ──
            if (health && !health->isAlive)
            {
                obj.velocity = {0, 0};
                obj.state    = AnimationState::Idle;
                continue;
            }

            // Feed scene pointer into any enemy controllers so they can find the player
            for (auto* comp : obj.components)
            {
                if (auto* ec = dynamic_cast<EnemyController*>(comp))
                    ec->sceneObjects = &objects;
            }

            // 1. apply gravity and run components
            obj.velocity.y += gravity * deltaTime;
            obj.Update(deltaTime); // components set velocity

            // Animation States Configuration
            const std::vector<SDL_FRect>* activeAnim = nullptr;
            switch (obj.state)
            {
                case AnimationState::Idle: activeAnim = &obj.framesIdle; break;
                case AnimationState::Walk: activeAnim = &obj.framesWalk; break;
                case AnimationState::Jump: activeAnim = &obj.framesJump; break;
            }

            // Animation (Frame cycling)
            if (activeAnim && !activeAnim->empty())
            {
                obj.animationTimer += deltaTime;
                if (obj.animationTimer >= obj.animationSpeed)
                {
                    obj.currentFrame++;
                    obj.animationTimer = 0.0f;

                    if (obj.currentFrame >= activeAnim->size())
                        obj.currentFrame = 0;
                }
            }
            else
            {
                obj.currentFrame = 0;
            }

            // =========================
            //  --- X AXIS ----------
            // =========================
            obj.position.x += obj.velocity.x * deltaTime;

            // X Tilemap Collision
            for (int y = 0; y < (int)tileMap.map.size(); y++)
            {
                for (int x = 0; x < (int)tileMap.map[y].size(); x++)
                {
                    if (tileMap.map[y][x] == 1)
                    {
                        float tileX = (float)(x * tileMap.tileSize);
                        float tileY = (float)(y * tileMap.tileSize);
                        float size = (float)tileMap.tileSize;

                        // AABB collision
                        bool collision =
                            obj.position.x < tileX + size &&
                            obj.position.x + obj.size.x > tileX &&
                            obj.position.y < tileY + size &&
                            obj.position.y + obj.size.y > tileY;

                        if (collision)
                        {
                            if (obj.velocity.x > 0) // moving right
                                obj.position.x = tileX - obj.size.x;
                            else if (obj.velocity.x < 0) // moving left
                                obj.position.x = tileX + size;

                            obj.velocity.x = 0;
                        }
                    }
                }
            }

            // X World Bounds
            if (obj.position.x <= 0)
            {
                obj.position.x  = 0;
                obj.velocity.x *= -1;
            }
            else if (obj.position.x + obj.size.x >= worldWidth)
            {
                obj.position.x  = (float)worldWidth - obj.size.x;
                obj.velocity.x *= -1;
            }


            // =========================
            //  --- Y AXIS ----------
            // =========================
            obj.position.y += obj.velocity.y * deltaTime;
            obj.isGrounded = false; // Reset before checking

            // Y Tilemap Collision
            for (int y = 0; y < (int)tileMap.map.size(); y++)
            {
                for (int x = 0; x < (int)tileMap.map[y].size(); x++)
                {
                    if (tileMap.map[y][x] == 1)
                    {
                        float tileX = (float)(x * tileMap.tileSize);
                        float tileY = (float)(y * tileMap.tileSize);
                        float size = (float)tileMap.tileSize;

                        // AABB collision
                        bool collision =
                            obj.position.x < tileX + size &&
                            obj.position.x + obj.size.x > tileX &&
                            obj.position.y < tileY + size &&
                            obj.position.y + obj.size.y > tileY;

                        if (collision)
                        {
                            if (obj.velocity.y > 0) // falling
                            {
                                obj.position.y = tileY - obj.size.y;
                                obj.velocity.y = 0;
                                obj.isGrounded = true;
                            }
                            else if (obj.velocity.y < 0) // jumping into ceiling
                            {
                                obj.position.y = tileY + size;
                                obj.velocity.y = 0;
                            }
                        }
                    }
                }
            }

            // Y World Bounds
            if (obj.position.y <= 0)
            {
                obj.position.y  = 0;
                obj.velocity.y *= -1;
            }
            else if (obj.position.y + obj.size.y >= worldHeight)
            {
                obj.position.y  = (float)worldHeight - obj.size.y;
                obj.velocity.y  = 0; // Stop falling (grounded)
                obj.isGrounded  = true;
            }
        }

        // Friction -- apply to any player-controlled object
        for (auto& obj : objects)
        {
            if (obj.GetComponent<PlayerController>())
            {
                obj.velocity.x *= 0.98f;
                obj.velocity.y *= 0.98f;
            }
        }

        // AABB collision (elastic, axis-aware)
        for (int i = 0; i < (int)objects.size(); i++)
        {
            for (int j = i + 1; j < (int)objects.size(); j++)
            {
                if (!CheckCollision(objects[i], objects[j])) continue;

                // ── GENERIC DAMAGE SYSTEM ────────────────────────────────────
                auto ApplyDamage = [&](GameObject& victim, GameObject& attacker)
                {
                    HealthComponent* vh = victim.GetComponent<HealthComponent>();
                    if (vh)
                    {
                        vh->TakeDamage(10); // Generic contact damage
                        camera.Shake(6.0f, 0.18f);
                        float kbDir = (victim.position.x < attacker.position.x) ? -1.0f : 1.0f;
                        victim.velocity.x = kbDir * 350.0f;
                        victim.velocity.y = -250.0f;
                    }
                };

                if (objects[i].isEnemy && !objects[j].isEnemy)
                    ApplyDamage(objects[j], objects[i]);
                else if (objects[j].isEnemy && !objects[i].isEnemy)
                    ApplyDamage(objects[i], objects[j]);

                // ── Physical pushback (existing elastic resolve) ──────────────
                float overlapX = std::min(objects[i].position.x + objects[i].size.x,
                                          objects[j].position.x + objects[j].size.x)
                               - std::max(objects[i].position.x, objects[j].position.x);

                float overlapY = std::min(objects[i].position.y + objects[i].size.y,
                                          objects[j].position.y + objects[j].size.y)
                               - std::max(objects[i].position.y, objects[j].position.y);

                if (overlapX < overlapY)
                {
                    float half = overlapX * 0.5f;
                    if (objects[i].position.x < objects[j].position.x)
                        { objects[i].position.x -= half; objects[j].position.x += half; }
                    else
                        { objects[i].position.x += half; objects[j].position.x -= half; }

                    std::swap(objects[i].velocity.x, objects[j].velocity.x);
                }
                else
                {
                    float half = overlapY * 0.5f;
                    if (objects[i].position.y < objects[j].position.y)
                        { objects[i].position.y -= half; objects[j].position.y += half; }
                    else
                        { objects[i].position.y += half; objects[j].position.y -= half; }

                    std::swap(objects[i].velocity.y, objects[j].velocity.y);
                }
            }
        }

        // ── GENERIC COMBAT DETECTION ──────────────────────────────────────────
        for (auto& attacker : objects)
        {
            auto* combat = attacker.GetComponent<CombatComponent>();
            if (!combat || !combat->isAttacking) continue;

            // Project hitbox
            float hbX = attacker.flipHorizontal 
                        ? attacker.position.x - combat->attackRange 
                        : attacker.position.x + attacker.size.x;

            for (auto& target : objects)
            {
                if (&attacker == &target) continue;
                
                auto* vh = target.GetComponent<HealthComponent>();
                if (!vh || !vh->isAlive) continue;

                // Simple AABB hit detection
                bool hitX = hbX < target.position.x + target.size.x && hbX + combat->attackRange > target.position.x;
                bool hitY = attacker.position.y < target.position.y + target.size.y && attacker.position.y + attacker.size.y > target.position.y;

                if (hitX && hitY)
                {
                    vh->TakeDamage(combat->damage);
                    camera.Shake(10.0f, 0.22f);

                    float kbDir = (target.position.x > attacker.position.x) ? 1.0f : -1.0f;
                    target.velocity.x = kbDir * 420.0f;
                    target.velocity.y = -180.0f;

                    if (!vh->isAlive) attacker.score += 100;
                }
            }
        }

        // ── REMOVE DEAD ENEMIES ───────────────────────────────────────────────
        objects.erase(
            std::remove_if(objects.begin(), objects.end(),
                [](const GameObject& o) { 
                    auto* h = o.GetComponent<HealthComponent>();
                    return o.isEnemy && h && !h->isAlive; 
                }),
            objects.end());
    }

    void Render(SDL_Renderer* renderer, const Vector2& cameraPos)
    {
        // Draw grid
        SDL_SetRenderDrawColor(renderer, 50, 50, 50, 255);

        // vertical lines
        for (int x = 0; x <= worldWidth; x += 100)
        {
            SDL_RenderLine(renderer, (float)x - cameraPos.x, 0.0f - cameraPos.y,
                                     (float)x - cameraPos.x, (float)worldHeight - cameraPos.y);
        }

        // horizontal lines
        for (int y = 0; y <= worldHeight; y += 100)
        {
            SDL_RenderLine(renderer, 0.0f - cameraPos.x, (float)y - cameraPos.y,
                                     (float)worldWidth - cameraPos.x, (float)y - cameraPos.y);
        }

        // --- DRAW TILEMAP ---
        for (int y = 0; y < (int)tileMap.map.size(); y++)
        {
            for (int x = 0; x < (int)tileMap.map[y].size(); x++)
            {
                if (tileMap.map[y][x] == 1)
                {
                    SDL_FRect tileRect =
                    {
                        (float)(x * tileMap.tileSize) - cameraPos.x,
                        (float)(y * tileMap.tileSize) - cameraPos.y,
                        (float)tileMap.tileSize,
                        (float)tileMap.tileSize
                    };

                    SDL_SetRenderDrawColor(renderer, 150, 80, 50, 255); // Brown dirt color
                    SDL_RenderFillRect(renderer, &tileRect);
                }
            }
        }

        // --- DRAW OBJECTS ---
        for (auto& obj : objects)
        {
            SDL_FRect rect = {
                obj.position.x - cameraPos.x,
                obj.position.y - cameraPos.y,
                obj.size.x,
                obj.size.y
            };

            if (obj.texture)
            {
                const std::vector<SDL_FRect>* activeAnim = nullptr;
                switch (obj.state)
                {
                    case AnimationState::Idle: activeAnim = &obj.framesIdle; break;
                    case AnimationState::Walk: activeAnim = &obj.framesWalk; break;
                    case AnimationState::Jump: activeAnim = &obj.framesJump; break;
                }
                SDL_FlipMode flip = obj.flipHorizontal ? SDL_FLIP_HORIZONTAL : SDL_FLIP_NONE;

                // ── Hit Flash: tint texture red (enemy) or white (player) ──────
                auto* health = obj.GetComponent<HealthComponent>();
                if (health && health->damageFlashTimer > 0.0f)
                {
                    if (obj.isEnemy)
                        SDL_SetTextureColorMod(obj.texture, 255, 60, 60);   // red flash
                    else
                        SDL_SetTextureColorMod(obj.texture, 255, 255, 255); // white flash
                }
                else
                {
                    SDL_SetTextureColorMod(obj.texture, 255, 255, 255);     // restore
                }

                if (activeAnim && !activeAnim->empty())
                {
                    int fr = obj.currentFrame;
                    if (fr < 0 || fr >= activeAnim->size()) fr = 0;
                    
                    SDL_FRect srcRect = (*activeAnim)[fr];
                    SDL_RenderTextureRotated(renderer, obj.texture, &srcRect, &rect, 0.0, nullptr, flip);
                }
                else
                {
                    // Not sliced yet, draw the whole texture as fallback
                    SDL_RenderTextureRotated(renderer, obj.texture, nullptr, &rect, 0.0, nullptr, flip);
                }
            }
            else
            {
                // Animation effect (color cycling)
                int color = 50 + obj.currentFrame * 50;

                SDL_SetRenderDrawColor(renderer, color, 0, 0, 255);
                SDL_RenderFillRect(renderer, &rect);
            }
        }

        // ── HealthComponent bars (world-space, above each entity) ─────────────
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        for (auto& obj : objects)
        {
            if (auto* hc = obj.GetComponent<HealthComponent>())
                hc->RenderBar(renderer, cameraPos);
        }

        // ── Attack hitbox visualisation (shows while swing is active) ─────────
        for (auto& obj : objects)
        {
            auto* combat = obj.GetComponent<CombatComponent>();
            if (!combat || !combat->isAttacking) continue;

            float hbX = obj.flipHorizontal
                        ? obj.position.x - combat->attackRange
                        : obj.position.x + obj.size.x;

            SDL_FRect hbRect = {
                hbX - cameraPos.x,
                obj.position.y - cameraPos.y,
                combat->attackRange,
                obj.size.y
            };

            // Pulsing orange fill — alpha proportional to remaining attack time
            float pulse = std::max(0.0f, combat->attackTimer / combat->attackDuration);
            SDL_SetRenderDrawColor(renderer, 255, 160, 40, (Uint8)(140 * pulse));
            SDL_RenderFillRect(renderer, &hbRect);

            // Bright border
            SDL_SetRenderDrawColor(renderer, 255, 220, 80, (Uint8)(220 * pulse));
            SDL_RenderRect(renderer, &hbRect);
        }

        // ── Game Over overlay — drawn INTO the game viewport texture ──────────
        GameObject* player = FindObjectWithComponent<PlayerController>();
        HealthComponent* ph = player ? player->GetComponent<HealthComponent>() : nullptr;
        
        if (ph && !ph->isAlive)
        {
            float sw = (float)screenWidth;
            float sh = (float)screenHeight;

            SDL_SetRenderDrawColor(renderer, 8, 0, 0, 185);
            SDL_FRect fullScreen = {0, 0, sw, sh};
            SDL_RenderFillRect(renderer, &fullScreen);

            // Accent separator lines
            SDL_SetRenderDrawColor(renderer, 255, 50, 50, 140);
            SDL_FRect topLine = {0, sh * 0.5f - 54, sw, 2};
            SDL_FRect botLine = {0, sh * 0.5f + 40, sw, 2};
            SDL_RenderFillRect(renderer, &topLine);
            SDL_RenderFillRect(renderer, &botLine);
        }
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
    }

private:
    static bool CheckCollision(const GameObject& a, const GameObject& b)
    {
        return (
            a.position.x            < b.position.x + b.size.x &&
            a.position.x + a.size.x > b.position.x            &&
            a.position.y            < b.position.y + b.size.y &&
            a.position.y + a.size.y > b.position.y
        );
    }
};
