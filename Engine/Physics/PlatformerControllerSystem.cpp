#include "pch.h"
#include "Physics/PlatformerControllerSystem.h"

#include "Physics/PhysicsLayers.h"
#include "Physics/PhysicsSystem.h"
#include "Scene/Components.h"

#include <algorithm>
#include <cmath>

namespace my2d
{
    static float MoveTowards(float current, float target, float maxDelta)
    {
        if (current < target)
            return std::min(current + maxDelta, target);
        if (current > target)
            return std::max(current - maxDelta, target);
        return target;
    }

    static bool GroundCheck(Engine& engine, const RigidBody2DComponent& rb, const BoxCollider2DComponent& bc, const PlatformerControllerComponent& pc)
    {
        if (!b2Body_IsValid(rb.bodyId)) return false;

        const float ppm = engine.PixelsPerMeter();
        const b2WorldId worldId = engine.GetPhysics().WorldId();

        // body center in meters
        const b2Vec2 center = b2Body_GetPosition(rb.bodyId);

        const float halfW = (bc.size.x * 0.5f) / ppm;
        const float halfH = (bc.size.y * 0.5f) / ppm;

        const float inset = (pc.groundRayInsetPx) / ppm;
        const float rayLen = (pc.groundCheckDistancePx) / ppm;

        // two rays: left & right foot (slightly inset)
        const b2Vec2 o1 = b2Vec2{ center.x - std::max(0.0f, halfW - inset), center.y + halfH - 0.001f };
        const b2Vec2 o2 = b2Vec2{ center.x + std::max(0.0f, halfW - inset), center.y + halfH - 0.001f };

        b2QueryFilter qf = b2DefaultQueryFilter();
        qf.categoryBits = PhysicsLayers::Player;
        qf.maskBits = PhysicsLayers::Environment;

        auto hitOk = [&](b2Vec2 origin)
            {
                b2RayResult res = b2World_CastRayClosest(worldId, origin, b2Vec2{ 0.0f, rayLen }, qf);
                if (!res.hit) return false;

                // With y+ down, ground normal should point UP => normal.y negative
                return res.normal.y < -0.2f;
            };

        return hitOk(o1) || hitOk(o2);
    }

    void PlatformerController_FixedUpdate(Engine& engine, Scene& scene, float fixedDt)
    {
        // Ensure runtime bodies exist (safe to call every tick)
        Physics_CreateRuntime(scene, engine.GetPhysics(), engine.PixelsPerMeter());

        auto& reg = scene.Registry();
        auto view = reg.view<TransformComponent, RigidBody2DComponent, BoxCollider2DComponent, PlatformerControllerComponent>();

        for (auto e : view)
        {
            auto& tc = view.get<TransformComponent>(e);
            auto& rb = view.get<RigidBody2DComponent>(e);
            auto& bc = view.get<BoxCollider2DComponent>(e);
            auto& pc = view.get<PlatformerControllerComponent>(e);

            if (!b2Body_IsValid(rb.bodyId))
                continue;

            // --- Grounding + timers ---
            const bool grounded = GroundCheck(engine, rb, bc, pc);
            pc.grounded = grounded;

            if (grounded)
                pc.coyoteTimer = pc.coyoteTime;
            else
                pc.coyoteTimer = std::max(0.0f, pc.coyoteTimer - fixedDt);

            // Jump buffer
            if (engine.GetInput().WasKeyPressed(pc.jump))
                pc.jumpBufferTimer = pc.jumpBufferTime;
            else
                pc.jumpBufferTimer = std::max(0.0f, pc.jumpBufferTimer - fixedDt);

            // --- Horizontal movement ---
            b2Vec2 v = b2Body_GetLinearVelocity(rb.bodyId);

            float axis = 0.0f;
            if (engine.GetInput().IsKeyDown(pc.left)) axis -= 1.0f;
            if (engine.GetInput().IsKeyDown(pc.right)) axis += 1.0f;

            const float ppm = engine.PixelsPerMeter();
            const float maxSpeed = pc.moveSpeedPx / ppm;     // m/s
            const float accel = pc.accelPx / ppm;           // m/s^2
            const float decel = pc.decelPx / ppm;

            const float targetX = axis * maxSpeed;
            const float maxDelta = (std::abs(axis) > 0.01f ? accel : decel) * fixedDt;
            v.x = MoveTowards(v.x, targetX, maxDelta);

            // --- Jump ---
            if (pc.jumpBufferTimer > 0.0f && pc.coyoteTimer > 0.0f)
            {
                const float jumpV = pc.jumpSpeedPx / ppm; // m/s
                v.y = -jumpV; // up is negative because y+ is down
                pc.jumpBufferTimer = 0.0f;
                pc.coyoteTimer = 0.0f;
            }

            b2Body_SetLinearVelocity(rb.bodyId, v);
        }

        // Sync transforms for rendering (so you don't have to do it in Game::OnUpdate)
        Physics_SyncTransforms(scene, engine.PixelsPerMeter());
    }
}