#include "pch.h"
#include "Physics/PlatformerControllerSystem.h"

#include "Physics/PhysicsLayers.h"
#include "Physics/PhysicsSystem.h"
#include "Scene/Components.h"

#include <algorithm>
#include <cmath>

namespace my2d
{

    struct GroundInfo
    {
        bool grounded = false;
        bool jumpable = false;
        b2Vec2 normal = b2Vec2{ 0, -1 };
        float slopeDeg = 0.0f;
    };


    static float MoveTowards(float current, float target, float maxDelta)
    {
        if (current < target)
            return std::min(current + maxDelta, target);
        if (current > target)
            return std::max(current - maxDelta, target);
        return target;
    }

    static GroundInfo GroundCheck(Engine& engine, const RigidBody2DComponent& rb, const BoxCollider2DComponent& bc, const PlatformerControllerComponent& pc)
    {
        GroundInfo best{};
        bool found = false;

        if (!b2Body_IsValid(rb.bodyId))
            return best;

        const float ppm = engine.PixelsPerMeter();
        const b2WorldId worldId = engine.GetPhysics().WorldId();

        const b2Vec2 center = b2Body_GetPosition(rb.bodyId);

        float halfW = (bc.size.x * 0.5f) / ppm;
        float halfH = (bc.size.y * 0.5f) / ppm;

        const float eps = 0.5f / ppm; // 0.5 px
        halfW += eps;
        halfH += eps;

        // “Skin” keeps ray origin slightly above the bottom so we don’t start inside geometry
        const float skin = (2.0f) / ppm; // 2 px
        const float rayLen = (pc.groundCheckDistancePx / ppm) + skin * 2.0f;

        // Clamp inset so we always actually cast near feet
        float inset = pc.groundRayInsetPx / ppm;
        inset = std::clamp(inset, 0.0f, halfW * 0.9f);
        const float footX = std::max(0.0f, halfW - inset);

        // Origins: left, center, right at “feet line”
        const b2Vec2 oL = b2Vec2{ center.x - footX, center.y + halfH - skin };
        const b2Vec2 oC = b2Vec2{ center.x,         center.y + halfH - skin };
        const b2Vec2 oR = b2Vec2{ center.x + footX, center.y + halfH - skin };

        b2QueryFilter qf = b2DefaultQueryFilter();
        qf.categoryBits = PhysicsLayers::Player;
        qf.maskBits = PhysicsLayers::Environment;

        auto consider = [&](b2Vec2 origin)
            {
                b2RayResult res = b2World_CastRayClosest(worldId, origin, b2Vec2{ 0.0f, rayLen }, qf);
                if (!res.hit) return;

                // Convert normal to slope angle vs “up”
                // y+ is down => up is (0,-1) so use -normal.y
                const float cosUp = std::clamp(-res.normal.y, 0.0f, 1.0f);
                const float slopeDeg = std::acos(cosUp) * 57.2957795f;

                GroundInfo gi;
                gi.normal = res.normal;
                gi.slopeDeg = slopeDeg;
                gi.grounded = (slopeDeg <= pc.maxGroundSlopeDeg);
                gi.jumpable = (slopeDeg <= pc.maxJumpSlopeDeg);

                if (!gi.grounded) return; // ignore near-vertical hits

                // Pick “best” = flattest (smallest slope)
                if (!found || gi.slopeDeg < best.slopeDeg)
                {
                    best = gi;
                    found = true;
                }
            };

        consider(oL);
        consider(oC);
        consider(oR);

        return best;
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
            GroundInfo gi = GroundCheck(engine, rb, bc, pc);
            pc.grounded = gi.grounded;
            pc.jumpableGround = gi.jumpable;

            if (pc.jumpableGround)
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

            // Cooldown timers
            pc.dashCooldownTimer = std::max(0.0f, pc.dashCooldownTimer - fixedDt);

            const bool hasDash = engine.GetWorldState().HasAbility(AbilityId::Dash);

            // Track facing based on input
            if (axis < -0.01f) pc.facing = -1;
            else if (axis > 0.01f) pc.facing = 1;

            // Start dash (only if ability unlocked)
            if (hasDash && !pc.isDashing && pc.dashCooldownTimer <= 0.0f && engine.GetInput().WasKeyPressed(pc.dash))
            {
                pc.isDashing = true;
                pc.dashTimer = pc.dashTime;
                pc.dashCooldownTimer = pc.dashCooldown;

                pc.dashDir = (axis < -0.01f) ? -1 : (axis > 0.01f ? 1 : pc.facing);
            }

            // During dash, override horizontal velocity
            if (pc.isDashing)
            {
                pc.dashTimer -= fixedDt;
                const float dashV = pc.dashSpeedPx / ppm; // m/s
                v.x = pc.dashDir * dashV;

                if (pc.dashTimer <= 0.0f)
                    pc.isDashing = false;
            }

            const float maxSpeed = pc.moveSpeedPx / ppm;     // m/s
            const float accel = pc.accelPx / ppm;           // m/s^2
            const float decel = pc.decelPx / ppm;

            if (!pc.isDashing)
            {
                const float targetX = axis * maxSpeed;
                const float maxDelta = (std::abs(axis) > 0.01f ? accel : decel) * fixedDt;
                v.x = MoveTowards(v.x, targetX, maxDelta);
            }

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
    }
}