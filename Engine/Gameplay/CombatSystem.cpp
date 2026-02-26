#include "pch.h"
#include "Gameplay/CombatSystem.h"

#include "Core/Engine.h"
#include "Scene/Scene.h"
#include "Scene/Components.h"
#include "Physics/PhysicsLayers.h"
#include "Physics/PhysicsSystem.h"

#include <vector>
#include <algorithm>
#include <cmath>

namespace my2d
{
    struct HitboxRuntime
    {
        entt::entity owner = entt::null;
        int damage = 1;
        float knockbackSpeed = 0.0f; // m/s
        float knockbackUp = 0.0f;    // m/s
        float victimInvuln = 0.25f;
        uint64_t targetMaskBits = 0;

        int dirX = 1;   // -1,0,1
        int dirY = 0;   // -1 up, 0 side, +1 down
        bool pogoOnDownHit = false;
        float pogoReboundSpeed = 0.0f; // m/s

        // prevent multi-hit from same swing
        std::vector<uint32_t> hitOnce;
    };

    static bool HitAlready(HitboxRuntime& rt, entt::entity victim)
    {
        const uint32_t v = (uint32_t)victim;
        return std::find(rt.hitOnce.begin(), rt.hitOnce.end(), v) != rt.hitOnce.end();
    }

    static void MarkHit(HitboxRuntime& rt, entt::entity victim)
    {
        rt.hitOnce.push_back((uint32_t)victim);
    }

    static int FacingFor(entt::registry& reg, entt::entity e)
    {
        if (reg.any_of<FacingComponent>(e))
            return reg.get<FacingComponent>(e).facing;

        if (reg.any_of<PlatformerControllerComponent>(e))
            return reg.get<PlatformerControllerComponent>(e).facing;

        return 1;
    }

    static void DestroyHitboxShape(MeleeAttackComponent& atk)
    {
        if (b2Shape_IsValid(atk.hitboxShapeId))
            b2DestroyShape(atk.hitboxShapeId, false);

        atk.hitboxShapeId = b2_nullShapeId;

        if (atk.runtimeUserData)
        {
            delete (HitboxRuntime*)atk.runtimeUserData;
            atk.runtimeUserData = nullptr;
        }
    }

    void Combat_PrePhysics(Engine& engine, Scene& scene, float fixedDt)
    {
        auto& reg = scene.Registry();

        // i-frames tick down
        {
            auto view = reg.view<InvincibilityComponent>();
            for (auto e : view)
            {
                auto& inv = view.get<InvincibilityComponent>(e);
                inv.timer = std::max(0.0f, inv.timer - fixedDt);
            }
        }

        // attacks
        auto view = reg.view<TransformComponent, RigidBody2DComponent, BoxCollider2DComponent, MeleeAttackComponent, TeamComponent>();
        for (auto e : view)
        {
            auto& rb = view.get<RigidBody2DComponent>(e);
            auto& bc = view.get<BoxCollider2DComponent>(e);
            auto& atk = view.get<MeleeAttackComponent>(e);

            // timers
            atk.cooldownTimer = std::max(0.0f, atk.cooldownTimer - fixedDt);

            if (atk.activeTimer > 0.0f)
            {
                atk.activeTimer -= fixedDt;
                if (atk.activeTimer <= 0.0f)
                    DestroyHitboxShape(atk);

                continue;
            }

            // no active hitbox
            if (b2Shape_IsValid(atk.hitboxShapeId))
                DestroyHitboxShape(atk);

            // start attack
            bool startAttack = false;
            if (atk.cooldownTimer <= 0.0f)
            {
                if (atk.useInput)
                    startAttack = engine.GetInput().WasKeyPressed(atk.attackKey);
                else
                    startAttack = atk.attackRequested;
            }

            if (startAttack)
            {
                atk.attackRequested = false;
                // ensure runtime exists
                Physics_CreateRuntime(scene, engine.GetPhysics(), engine.PixelsPerMeter());
                if (!b2Body_IsValid(rb.bodyId))
                    continue;

                const float ppm = engine.PixelsPerMeter();
                const int facing = FacingFor(reg, e);

                // Decide attack direction (default = horizontal by facing)
                int dirX = facing;
                int dirY = 0;

                // If you have a platformer controller, use grounded state for down-attack restriction
                bool grounded = false;
                if (reg.any_of<PlatformerControllerComponent>(e))
                    grounded = reg.get<PlatformerControllerComponent>(e).grounded;

                glm::vec2 sizePx = atk.hitboxSizePx;
                glm::vec2 offsetPx = atk.hitboxOffsetPx;

                // Aim up/down (hold aim key + press attack)
                if (atk.allowAimUp && engine.GetInput().IsKeyDown(atk.aimUpKey))
                {
                    dirX = 0;
                    dirY = -1;

                    // Use custom up size/offset if provided
                    if (atk.hitboxSizeUpPx.x > 0 && atk.hitboxSizeUpPx.y > 0) sizePx = atk.hitboxSizeUpPx;
                    offsetPx = atk.hitboxOffsetUpPx;
                }
                else if (atk.allowAimDown && engine.GetInput().IsKeyDown(atk.aimDownKey) &&
                    (atk.allowDownAttackOnGround || !grounded))
                {
                    dirX = 0;
                    dirY = +1;

                    if (atk.hitboxSizeDownPx.x > 0 && atk.hitboxSizeDownPx.y > 0) sizePx = atk.hitboxSizeDownPx;
                    offsetPx = atk.hitboxOffsetDownPx;
                }

                // Convert to meters
                const float halfW = (sizePx.x * 0.5f) / ppm;
                const float halfH = (sizePx.y * 0.5f) / ppm;

                // For vertical attacks, you often want a slight forward bias; multiply X offset by facing
                const float xBias = (float)facing;

                b2Vec2 centerLocal = b2Vec2{
                    (offsetPx.x * xBias) / ppm,
                    (offsetPx.y) / ppm
                };

                // Offset box polygon (on the same body)
                b2Polygon poly = b2MakeOffsetBox(halfW, halfH, centerLocal, b2MakeRot(0.0f));

                b2ShapeDef sd = b2DefaultShapeDef();
                sd.isSensor = true;
                sd.enableSensorEvents = true;                 // required for sensor events
                sd.filter.categoryBits = PhysicsLayers::Hitbox;
                sd.filter.maskBits = atk.targetMaskBits;
                sd.density = 0.0f;

                atk.hitboxShapeId = b2CreatePolygonShape(rb.bodyId, &sd, &poly);

                // attach runtime payload to sensor shape
                HitboxRuntime* rt = new HitboxRuntime();
                rt->owner = (entt::entity)e;
                rt->damage = atk.damage;
                rt->knockbackSpeed = atk.knockbackSpeedPx / ppm;
                rt->knockbackUp = atk.knockbackUpPx / ppm;
                rt->victimInvuln = atk.victimInvuln;
                rt->targetMaskBits = atk.targetMaskBits;
                rt->dirX = dirX;
                rt->dirY = dirY;
                rt->pogoOnDownHit = atk.pogoOnDownHit;
                rt->pogoReboundSpeed = atk.pogoReboundSpeedPx / ppm;

                atk.runtimeUserData = rt;
                b2Shape_SetUserData(atk.hitboxShapeId, rt);

                atk.activeTimer = atk.activeTime;
                atk.cooldownTimer = atk.cooldown;
            }
        }
    }

    void Combat_PostPhysics(Engine& engine, Scene& scene, float /*fixedDt*/)
    {
        auto& reg = scene.Registry();

        b2WorldId worldId = engine.GetPhysics().WorldId();
        b2SensorEvents ev = b2World_GetSensorEvents(worldId);

        std::vector<entt::entity> toKill;

        for (int i = 0; i < ev.beginCount; ++i)
        {
            const b2SensorBeginTouchEvent& begin = ev.beginEvents[i];

            if (!b2Shape_IsValid(begin.sensorShapeId) || !b2Shape_IsValid(begin.visitorShapeId))
                continue;

            void* sensorUd = b2Shape_GetUserData(begin.sensorShapeId);
            if (!sensorUd)
                continue;

            auto* rt = (HitboxRuntime*)sensorUd;

            void* visitorUd = b2Shape_GetUserData(begin.visitorShapeId);
            if (!visitorUd)
                continue;

            entt::entity victim = (entt::entity)(uint32_t)(uintptr_t)visitorUd;

            if (victim == entt::null || victim == rt->owner)
                continue;

            if (!reg.valid(victim) || !reg.valid(rt->owner))
                continue;

            // require hurtbox + health
            if (!reg.any_of<HurtboxComponent, HealthComponent, TeamComponent>(victim))
                continue;

            auto& hurt = reg.get<HurtboxComponent>(victim);
            if (!hurt.enabled)
                continue;

            // team check
            auto ownerTeam = reg.any_of<TeamComponent>(rt->owner) ? reg.get<TeamComponent>(rt->owner).team : Team::Neutral;
            auto victimTeam = reg.get<TeamComponent>(victim).team;
            if (ownerTeam == victimTeam)
                continue;

            // prevent multi-hit from same swing
            if (HitAlready(*rt, victim))
                continue;

            // invuln check
            if (reg.any_of<InvincibilityComponent>(victim))
            {
                if (reg.get<InvincibilityComponent>(victim).timer > 0.0f)
                    continue;
            }

            // apply damage
            auto& hp = reg.get<HealthComponent>(victim);
            hp.hp -= rt->damage;

            // set invuln
            auto& inv = reg.emplace_or_replace<InvincibilityComponent>(victim);
            inv.timer = rt->victimInvuln;

            // knockback
            if (reg.any_of<RigidBody2DComponent>(victim))
            {
                auto& vrb = reg.get<RigidBody2DComponent>(victim);
                if (b2Body_IsValid(vrb.bodyId))
                {
                    const b2Vec2 vpos = b2Body_GetPosition(vrb.bodyId);
                    float dir = 1.0f;

                    if (reg.any_of<RigidBody2DComponent>(rt->owner))
                    {
                        auto& orb = reg.get<RigidBody2DComponent>(rt->owner);
                        if (b2Body_IsValid(orb.bodyId))
                        {
                            const b2Vec2 opos = b2Body_GetPosition(orb.bodyId);
                            dir = (vpos.x >= opos.x) ? 1.0f : -1.0f;
                        }
                    }

                    b2Vec2 vel = b2Body_GetLinearVelocity(vrb.bodyId);

                    if (rt->dirY == -1) // up attack
                    {
                        vel.x = 0.0f;
                        vel.y = -rt->knockbackUp;
                    }
                    else if (rt->dirY == +1) // down attack
                    {
                        vel.x = 0.0f;
                        vel.y = +rt->knockbackUp; // down is +Y
                    }
                    else // side attack
                    {
                        vel.x = dir * rt->knockbackSpeed;
                        vel.y = -rt->knockbackUp;
                    }

                    b2Body_SetLinearVelocity(vrb.bodyId, vel);
                }
                // Optional pogo rebound when down-attacking something (only makes sense mid-air)
                if (rt->dirY == +1 && rt->pogoOnDownHit && rt->pogoReboundSpeed > 0.0f)
                {
                    if (reg.any_of<RigidBody2DComponent>(rt->owner) && reg.any_of<PlatformerControllerComponent>(rt->owner))
                    {
                        auto& ownerRb = reg.get<RigidBody2DComponent>(rt->owner);
                        auto& ownerPc = reg.get<PlatformerControllerComponent>(rt->owner);

                        if (!ownerPc.grounded && b2Body_IsValid(ownerRb.bodyId))
                        {
                            b2Vec2 ov = b2Body_GetLinearVelocity(ownerRb.bodyId);
                            ov.y = -rt->pogoReboundSpeed;
                            b2Body_SetLinearVelocity(ownerRb.bodyId, ov);
                        }
                    }
                }
            }

            MarkHit(*rt, victim);

            if (hp.hp <= 0)
                toKill.push_back(victim);
        }

        // kill after processing events
        for (auto e : toKill)
        {
            if (!reg.valid(e)) continue;
            Physics_DestroyRuntimeForEntity(scene, e);
            reg.destroy(e);
        }
    }
}