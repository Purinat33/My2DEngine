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
            if (atk.cooldownTimer <= 0.0f && engine.GetInput().WasKeyPressed(atk.attackKey))
            {
                // ensure runtime exists
                Physics_CreateRuntime(scene, engine.GetPhysics(), engine.PixelsPerMeter());
                if (!b2Body_IsValid(rb.bodyId))
                    continue;

                const float ppm = engine.PixelsPerMeter();
                const int facing = FacingFor(reg, e);

                const float halfW = (atk.hitboxSizePx.x * 0.5f) / ppm;
                const float halfH = (atk.hitboxSizePx.y * 0.5f) / ppm;

                b2Vec2 centerLocal = b2Vec2{
                    (atk.hitboxOffsetPx.x * (float)facing) / ppm,
                    (atk.hitboxOffsetPx.y) / ppm
                };

                // Build offset box polygon (local-space offset on the same body)
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
                    vel.x = dir * rt->knockbackSpeed;
                    vel.y = -rt->knockbackUp;
                    b2Body_SetLinearVelocity(vrb.bodyId, vel);
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