#pragma once
#include <vector>
#include <algorithm>
#include <SDL3/SDL.h>
#include "GameObject.h"
#include "TileMap.h"

class Scene
{
public:
    std::vector<GameObject> objects;
    TileMap tileMap;

    int screenWidth  = 800;
    int screenHeight = 600;

    int worldWidth   = 2000;
    int worldHeight  = 2000;

    void Update(float deltaTime)
    {
        float gravity = 1500.0f; // Snappy platformer gravity

        // Move all objects + resolve collisions using split-axis logic
        for (auto& obj : objects)
        {
            // 1. apply gravity and run components
            obj.velocity.y += gravity * deltaTime;
            obj.Update(deltaTime); // components set velocity

            // Animation (Frame cycling)
            obj.animationTimer += deltaTime;
            if (obj.animationTimer >= obj.animationSpeed)
            {
                obj.currentFrame++;
                obj.animationTimer = 0.0f;

                if (obj.currentFrame > obj.endFrame || obj.currentFrame < obj.startFrame)
                    obj.currentFrame = obj.startFrame;
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

        // Friction -- player (index 0) only
        if (!objects.empty())
        {
            objects[0].velocity.x *= 0.98f;
            objects[0].velocity.y *= 0.98f;
        }

        // AABB collision (elastic, axis-aware)
        for (int i = 0; i < (int)objects.size(); i++)
        {
            for (int j = i + 1; j < (int)objects.size(); j++)
            {
                if (!CheckCollision(objects[i], objects[j])) continue;

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
                SDL_FRect srcRect =
                {
                    (float)(obj.currentFrame * obj.frameWidth),
                    0.0f,
                    (float)obj.frameWidth,
                    (float)obj.frameHeight
                };

                SDL_RenderTexture(renderer, obj.texture, &srcRect, &rect);
            }
            else
            {
                // Animation effect (color cycling)
                int color = 50 + obj.currentFrame * 50;

                SDL_SetRenderDrawColor(renderer, color, 0, 0, 255);
                SDL_RenderFillRect(renderer, &rect);
            }
        }
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
