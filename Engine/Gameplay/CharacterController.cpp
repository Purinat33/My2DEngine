#include "pch.h"
#include "Gameplay/CharacterController.h"

namespace eng::gameplay
{
    
    void CharacterController::FixedUpdate(float dt, float moveX, bool jumpPressed, const eng::world::Tilemap& map)
    {
        m_wasGrounded = m_grounded;

        // timers
        if (m_grounded) m_coyote = m_cfg.coyoteTime;
        else m_coyote = std::max(0.0f, m_coyote - dt);

        if (jumpPressed) m_jumpBuf = m_cfg.jumpBuffer;
        else m_jumpBuf = std::max(0.0f, m_jumpBuf - dt);

        // horizontal movement
        float targetVx = eng::Clamp(moveX, -1.0f, 1.0f) * m_cfg.moveSpeed;
        float accel = m_grounded ? m_cfg.accelGround : m_cfg.accelAir;
        m_vel.x = eng::MoveTowards(m_vel.x, targetVx, accel * dt);

        // gravity
        m_vel.y += m_cfg.gravity * dt;
        if (m_vel.y > m_cfg.maxFall) m_vel.y = m_cfg.maxFall;

        // jump (Y+ down => jump is negative Y)
        if (m_jumpBuf > 0.0f && m_coyote > 0.0f)
        {
            m_vel.y = -m_cfg.jumpSpeed;
            m_grounded = false;
            m_coyote = 0.0f;
            m_jumpBuf = 0.0f;
        }

        const float dx = m_vel.x * dt;
        const float dy = m_vel.y * dt;

        // --- Move X, resolve, step-up if we hit a wall while grounded-ish ---
        const eng::Vec2f start = m_cap.center;
        m_cap.center.x += dx;

        SolveResult resX = SolveCollisions(map);

        const bool groundedish = m_wasGrounded || resX.grounded;
        const bool onSlope = resX.grounded && std::fabs(resX.groundNormal.x) > 0.2f; // slope-ish normal
        if (groundedish && !onSlope && resX.hitWall && std::fabs(dx) > 0.001f)
        {
            // Retry with step-up from original position (pre-X move)
            m_cap.center = start;

            // Try step-up move; if it fails, fall back to normal X move+solve
            if (!TryStepUp(map, dx))
            {
                m_cap.center = start;
                m_cap.center.x += dx;
                SolveCollisions(map);
            }
        }

        // --- Move Y, resolve ---
        m_cap.center.y += dy;
        SolveResult resY = SolveCollisions(map);

        // Ground snap helps slope seams + tile boundaries
        if (!m_grounded && m_wasGrounded && m_vel.y >= 0.0f)
        {
            if (TryGroundSnap(map))
            {
                if (m_vel.y > 0.0f) m_vel.y = 0.0f;
            }
        }

        // slope behavior: if grounded, keep velocity along slope tangent
        if (m_grounded)
        {
            eng::Vec2f n = m_groundNormal;
            eng::Vec2f t = eng::Normalize(eng::Vec2f{ -n.y, n.x });
            float speedT = eng::Dot(m_vel, t);
            m_vel = t * speedT;
        }

        if (m_grounded && m_vel.y >= 0.0f)
        {
            TryGroundSnap(map);
        }

        (void)resY;
    }

    CharacterController::SolveResult CharacterController::SolveCollisions(const eng::world::Tilemap& map)
    {
        SolveResult out{};
        out.grounded = false;
        out.groundNormal = { 0, -1 };

        for (int iter = 0; iter < m_cfg.solverIterations; ++iter)
        {
            eng::collision::Contact c{};
            if (!eng::collision::FindBestTileContact(m_cap, map, c))
                break;

            // push out
            m_cap.center = m_cap.center + c.normal * c.penetration;

            // remove velocity into the surface
            float vn = eng::Dot(m_vel, c.normal);
            if (vn < 0.0f)
                m_vel = m_vel - c.normal * vn;

            // classify contact
            if (std::fabs(c.normal.x) >= 0.90f && std::fabs(c.normal.y) <= 0.25f)
                out.hitWall = true;

            if (c.normal.y >= m_cfg.ceilingNormalY)
                out.hitCeiling = true;

            if (c.normal.y <= m_cfg.groundNormalY)
            {
                out.grounded = true;
                out.groundNormal = c.normal;
            }
        }

        // commit grounded state
        m_grounded = out.grounded;
        m_groundNormal = out.groundNormal;

        return out;
    }

    bool CharacterController::TryStepUp(const eng::world::Tilemap& map, float dx)
    {
        const eng::Vec2f start = m_cap.center;
        const eng::Vec2f velSaved = m_vel;

        // 1) step up
        m_cap.center.y -= m_cfg.stepHeightPx;
        SolveResult resUp = SolveCollisions(map);

        // If we immediately hit ceiling hard, abort
        if (resUp.hitCeiling)
        {
            m_cap.center = start;
            m_vel = velSaved;
            return false;
        }

        // 2) move horizontally at stepped height
        m_cap.center.x += dx;
        SolveResult resStepX = SolveCollisions(map);

        if (resStepX.hitWall)
        {
            // step failed, revert
            m_cap.center = start;
            m_vel = velSaved;
            return false;
        }

        // 3) optional: try snap down (helps settle onto the new surface)
        TryGroundSnap(map);
        return true;
    }

    bool CharacterController::TryGroundSnap(const eng::world::Tilemap& map)
    {
        eng::collision::Capsule probe = m_cap;
        probe.center.y += m_cfg.snapDistance;

        eng::collision::Contact c{};
        if (!eng::collision::FindBestTileContact(probe, map, c))
            return false;

        if (c.normal.y <= m_cfg.groundNormalY)
        {
            // place snapped capsule, then resolve it out
            m_cap = probe;
            m_cap.center = m_cap.center + c.normal * c.penetration;

            m_grounded = true;
            m_groundNormal = c.normal;
            m_coyote = m_cfg.coyoteTime;
            return true;
        }
        return false;
    }

    void CharacterController::DebugDraw(eng::Renderer2D& r, const eng::world::Tilemap& map) const
    {
        // Draw collision polys
        for (int y = 0; y < map.height; ++y)
            for (int x = 0; x < map.width; ++x)
            {
                auto t = map.Get(x, y);
                if (t == eng::world::ColliderType::Empty) continue;

                auto p = map.GetColliderPolyWorldPx(x, y);
                if (p.count >= 3)
                    r.DrawPolyLine(p.v.data(), p.count, eng::Color{ 80, 200, 120, 255 }, true);
            }

        // Draw capsule approximation: line segment + two “circles” (as a 12-gon)
        eng::Vec2f a, b;
        m_cap.SegmentEndpoints(a, b);

        r.DrawLine((int)a.x, (int)a.y, (int)b.x, (int)b.y, eng::Color{ 255, 255, 0, 255 });

        auto drawCircle = [&](eng::Vec2f c)
            {
                static constexpr int N = 12;
                eng::Vec2f pts[N];
                for (int i = 0; i < N; ++i)
                {
                    float t = (float)i / (float)N * 6.2831853f;
                    pts[i] = eng::Vec2f{ c.x + std::cos(t) * m_cap.radius, c.y + std::sin(t) * m_cap.radius };
                }
                r.DrawPolyLine(pts, N, eng::Color{ 255, 255, 0, 255 }, true);
            };

        drawCircle(a);
        drawCircle(b);

        // Draw ground normal
        if (m_grounded)
        {
            eng::Vec2f p = m_cap.center;
            eng::Vec2f n = m_groundNormal;
            eng::Vec2f end = p + n * 60.0f;
            r.DrawLine((int)p.x, (int)p.y, (int)end.x, (int)end.y, eng::Color{ 80, 160, 255, 255 });
        }
    }
}