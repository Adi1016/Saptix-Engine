#pragma once
#include "Component.h"
#include <string>

// ─────────────────────────────────────────────────────────────────────────────
//  CombatComponent
//  A component that manages attacking state, damage, range, and cooldowns.
//  Can be attached to ANY GameObject: player, boss, enemy units.
// ─────────────────────────────────────────────────────────────────────────────
class CombatComponent : public Component
{
public:
    std::string GetName() const override { return "CombatComponent"; }

    // ── Combat Settings (exposed in inspector) ──────────────────────────────
    int   damage           = 25;     // Damage dealt per hit
    float attackRange      = 90.0f;  // Hitbox reach (horizontal)
    float attackDuration   = 0.18f;  // Active hitbox window (seconds)
    float attackCooldown   = 0.45f;  // Cooldown between attacks

    // ── Combat State (internal processing) ──────────────────────────────────
    bool  isAttacking         = false;
    float attackTimer         = 0.0f;
    float attackCooldownTimer = 0.0f;

    // Trigger an attack if not on cooldown
    void Attack()
    {
        if (isAttacking || attackCooldownTimer > 0.0f) return;

        isAttacking         = true;
        attackTimer         = attackDuration;
        attackCooldownTimer = attackCooldown;
    }

    void Update(float deltaTime) override
    {
        if (attackCooldownTimer > 0.0f)
            attackCooldownTimer -= deltaTime;

        if (isAttacking)
        {
            attackTimer -= deltaTime;
            if (attackTimer <= 0.0f)
                isAttacking = false;
        }
    }
};
