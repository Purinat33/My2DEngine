#pragma once
#include "Core/Types.h"
#include <box2d/box2d.h>

namespace eng::physics
{
    constexpr float PixelsPerMeter = 100.0f;

    inline float ToMeters(float px) { return px / PixelsPerMeter; }
    inline float ToPixels(float m) { return m * PixelsPerMeter; }

    struct Body
    {
        b2BodyId id = b2_nullBodyId;
    };

    class PhysicsWorld
    {
    public:
        PhysicsWorld();                         // default gravity (0, +9.8), Y down
        explicit PhysicsWorld(eng::Vec2f gravity);
        ~PhysicsWorld();

        PhysicsWorld(PhysicsWorld&&) noexcept;
        PhysicsWorld& operator=(PhysicsWorld&&) noexcept;

        PhysicsWorld(const PhysicsWorld&) = delete;
        PhysicsWorld& operator=(const PhysicsWorld&) = delete;

        void SetGravity(eng::Vec2f gravity);
        void Step(float dt, int subStepCount = 4);

        Body CreateStaticBoxPx(float cxPx, float cyPx, float wPx, float hPx, float friction = 0.8f);
        Body CreateDynamicBoxPx(float cxPx, float cyPx, float wPx, float hPx,
            float density = 1.0f, float friction = 0.3f, float restitution = 0.0f);

        eng::Vec2f GetPositionPx(Body b) const;
        bool IsValid(Body b) const;

    private:
        b2WorldId m_world = b2_nullWorldId;
    };
}