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

        // integrate
        m_cap.center.x += m_vel.x * dt;
        m_cap.center.y += m_vel.y * dt;

        // collision solve + ground detect
        SolveCollisions(map);

        // ground snap (fixes slope seams + tile boundaries)
        if (!m_grounded && m_wasGrounded && m_vel.y >= 0.0f)
        {
            if (TryGroundSnap(map))
            {
                // Once snapped, kill tiny downward velocity
                if (m_vel.y > 0.0f) m_vel.y = 0.0f;
            }
        }

        // slope behavior: if grounded, project velocity onto slope tangent
        if (m_grounded)
        {
            eng::Vec2f n = m_groundNormal;
            eng::Vec2f t = eng::Normalize(eng::Vec2f{ -n.y, n.x }); // tangent

            float speedT = eng::Dot(m_vel, t);
            m_vel = t * speedT;
        }
    }

    void CharacterController::SolveCollisions(const eng::world::Tilemap& map)
    {
        m_grounded = false;
        m_groundNormal = { 0, -1 };

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

            // ground check
            if (c.normal.y <= m_cfg.groundNormalY)
            {
                m_grounded = true;
                m_groundNormal = c.normal;
            }
        }
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