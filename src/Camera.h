#pragma once
#include "Vector2.h"
#include <cstdlib>  // rand()
#include <cmath>

struct Camera
{
    Vector2 position;

    // ── Screen Shake ──────────────────────────────────────────────
    float shakeTimer    = 0.0f;   // Seconds of shake remaining
    float shakeStrength = 0.0f;   // Max pixel offset per axis
    float shakeFalloff  = 6.0f;   // How fast strength fades (higher = faster decay)

    // Call this every frame before passing camera to Render
    void Update(float deltaTime)
    {
        if (shakeTimer > 0.0f)
            shakeTimer -= deltaTime;
    }

    // Trigger a shake: strength=pixels, duration=seconds
    void Shake(float strength, float duration)
    {
        // Only upgrade, never downgrade an existing shake
        if (strength > shakeStrength || shakeTimer <= 0.0f)
        {
            shakeStrength = strength;
            shakeTimer    = duration;
        }
    }

    // Returns the camera position + random shake offset (call once per Render)
    Vector2 GetRenderPosition() const
    {
        if (shakeTimer <= 0.0f) return position;

        float t   = shakeTimer;           // remaining time drives amplitude
        float amp = shakeStrength * (t > 0 ? std::min(t * shakeFalloff, 1.0f) : 0.0f);

        float ox = ((float)(rand() % 201) - 100.0f) / 100.0f * amp;
        float oy = ((float)(rand() % 201) - 100.0f) / 100.0f * amp;
        return { position.x + ox, position.y + oy };
    }
};
