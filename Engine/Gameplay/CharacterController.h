#pragma once
#include "Core/Types.h"
#include "Core/Math2D.h"
#include "Collision/Collision2D.h"
#include "World/Tilemap.h"
#include "Graphics/Renderer2D.h"

namespace eng::gameplay
{
    struct ControllerConfig
    {
        float radiusPx = 18.0f;
        float halfHeightPx = 44.0f;

        float moveSpeed = 520.0f;      // px/s
        float accelGround = 7000.0f;   // px/s^2
        float accelAir = 4200.0f;      // px/s^2

        float gravity = 2600.0f;       // px/s^2 (Y+ down)
        float maxFall = 1800.0f;       // px/s

        float jumpSpeed = 980.0f;      // px/s (upwards => negative Y)
        float coyoteTime = 0.10f;      // seconds
        float jumpBuffer = 0.10f;      // seconds

        float snapDistance = 4.0f;     // px
        int solverIterations = 6;
        float groundNormalY = -0.6f;   // normals with y <= this are “ground”

        float stepHeightPx = 12.0f;     // how high we can "step" up (for 64px tiles, 10-14 is good)
        float wallNormalX = 0.65f;      // |normal.x| above this counts as a wall hit
        float ceilingNormalY = 0.65f;   // normal.y >= this counts as ceiling
    };

    class CharacterController
    {
    public:
        explicit CharacterController(ControllerConfig cfg = {}) : m_cfg(cfg)
        {
            m_cap.radius = cfg.radiusPx;
            m_cap.halfHeight = cfg.halfHeightPx;
        }

        void SetPositionPx(eng::Vec2f p) { m_cap.center = p; }
        void TeleportPx(eng::Vec2f p)
        {
            m_cap.center = p;
            m_vel = { 0,0 };
            m_grounded = false;
            m_groundNormal = { 0,-1 };
            m_coyote = 0.0f;
            m_jumpBuf = 0.0f;
            m_wasGrounded = false;
        }
        eng::Vec2f GetPositionPx() const { return m_cap.center; }
        eng::Vec2f GetVelocityPx() const { return m_vel; }
        bool IsGrounded() const { return m_grounded; }
        eng::Vec2f GroundNormal() const { return m_groundNormal; }
        eng::RectF GetBoundsPx() const
        {
            return eng::RectF{
                m_cap.center.x - m_cap.radius,
                m_cap.center.y - m_cap.halfHeight,
                m_cap.radius * 2.0f,
                m_cap.halfHeight * 2.0f
            };
        }

        // moveX: -1..+1, jumpPressed: edge or “pressed this frame”
        void FixedUpdate(float dt, float moveX, bool jumpPressed, const eng::world::Tilemap& map);

        void DebugDraw(eng::Renderer2D& r, const eng::world::Tilemap& map) const;

    private:
        struct SolveResult
        {
            bool hitWall = false;
            bool hitCeiling = false;
            bool grounded = false;
            eng::Vec2f groundNormal{ 0, -1 };
        };

        SolveResult SolveCollisions(const eng::world::Tilemap& map);
        bool TryStepUp(const eng::world::Tilemap& map, float dx);

        bool TryGroundSnap(const eng::world::Tilemap& map);

        ControllerConfig m_cfg{};
        eng::collision::Capsule m_cap{};
        eng::Vec2f m_vel{ 0,0 };

        bool m_grounded = false;
        eng::Vec2f m_groundNormal{ 0, -1 };

        float m_coyote = 0.0f;
        float m_jumpBuf = 0.0f;

        bool m_wasGrounded = false;
    };
}