#include "pch.h"
#include "Gameplay/EnemyAISystem.h"

#include "Core/Engine.h"
#include "Scene/Scene.h"
#include "Scene/Entity.h"
#include "Scene/Components.h"
#include "Physics/PhysicsLayers.h"

#include <algorithm>
#include <cmath>

namespace my2d
{
    static bool RayHitEnv(b2WorldId worldId, b2Vec2 origin, b2Vec2 delta, uint64_t selfCategory)
    {
        b2QueryFilter qf = b2DefaultQueryFilter();
        qf.categoryBits = selfCategory;
        qf.maskBits = PhysicsLayers::Environment;

        b2RayResult res = b2World_CastRayClosest(worldId, origin, delta, qf);
        return res.hit;
    }

    void EnemyAI_FixedUpdate(Engine& engine, Scene& scene, float fixedDt, Entity player)
    {
        if (!player) return;

        auto& reg = scene.Registry();
        if (!reg.valid(player.Handle())) return;

        const auto& playerT = reg.get<TransformComponent>(player.Handle());

        b2WorldId worldId = engine.GetPhysics().WorldId();
        const float ppm = engine.PixelsPerMeter();

        auto view = reg.view<TransformComponent, RigidBody2DComponent, BoxCollider2DComponent, EnemyAIComponent, FacingComponent, TeamComponent>();
        for (auto e : view)
        {
            auto& tc = view.get<TransformComponent>(e);
            auto& rb = view.get<RigidBody2DComponent>(e);
            auto& bc = view.get<BoxCollider2DComponent>(e);
            auto& ai = view.get<EnemyAIComponent>(e);
            auto& face = view.get<FacingComponent>(e);
            auto& team = view.get<TeamComponent>(e);

            if (team.team != Team::Enemy) continue;
            if (!b2Body_IsValid(rb.bodyId)) continue;

            // Aggro logic by distance (simple + fast)
            const float dx = playerT.position.x - tc.position.x;
            const float dy = playerT.position.y - tc.position.y;
            const float dist = std::sqrt(dx * dx + dy * dy);

            if (!ai.aggro && dist <= ai.aggroRangePx) ai.aggro = true;
            if (ai.aggro && dist >= ai.deaggroRangePx) ai.aggro = false;

            // Face toward player when aggro
            if (ai.aggro && std::abs(dx) > 1.0f)
                face.facing = (dx >= 0.0f) ? 1 : -1;

            // Attack request
            if (ai.aggro && dist <= ai.attackRangePx && reg.any_of<MeleeAttackComponent>(e))
            {
                auto& atk = reg.get<MeleeAttackComponent>(e);
                atk.useInput = false;               // AI controlled
                atk.attackRequested = true;         // CombatSystem will consume if off cooldown
            }

            // Movement velocity
            b2Vec2 v = b2Body_GetLinearVelocity(rb.bodyId);

            float desiredX = 0.0f;

            if (ai.aggro && ai.canChase)
            {
                desiredX = (float)face.facing * (ai.chaseSpeedPx / ppm);
            }
            else if (ai.canPatrol)
            {
                // Basic ledge/wall checks to flip direction
                const b2Vec2 center = b2Body_GetPosition(rb.bodyId);

                const float halfW = (bc.size.x * 0.5f) / ppm;
                const float halfH = (bc.size.y * 0.5f) / ppm;

                const float fwd = ai.wallCheckForwardPx / ppm;
                const float down = ai.ledgeCheckDownPx / ppm;

                // Cast forward from mid-body to detect walls
                const b2Vec2 wallOrigin = b2Vec2{ center.x + (float)face.facing * halfW, center.y };
                const bool wallHit = RayHitEnv(worldId, wallOrigin, b2Vec2{ (float)face.facing * fwd, 0.0f }, PhysicsLayers::Enemy);

                // Cast down from front foot to detect ledge
                const b2Vec2 footOrigin = b2Vec2{ center.x + (float)face.facing * (halfW - 0.01f), center.y + halfH };
                const bool groundHit = RayHitEnv(worldId, footOrigin, b2Vec2{ 0.0f, down }, PhysicsLayers::Enemy);

                if (wallHit || !groundHit)
                    face.facing *= -1;

                desiredX = (float)face.facing * (ai.patrolSpeedPx / ppm);
            }

            // Apply desired x, preserve y
            v.x = desiredX;
            b2Body_SetLinearVelocity(rb.bodyId, v);
        }
    }
}