#include "pch.h"
#include "Physics/PhysicsWorld.h"

namespace eng::physics
{
    PhysicsWorld::PhysicsWorld()
        : PhysicsWorld(eng::Vec2f{ 0.0f, 9.8f })
    {
    }

    PhysicsWorld::PhysicsWorld(eng::Vec2f gravity)
    {
        b2WorldDef worldDef = b2DefaultWorldDef();
        worldDef.gravity = b2Vec2{ gravity.x, gravity.y };
        m_world = b2CreateWorld(&worldDef);
    }

    PhysicsWorld::~PhysicsWorld()
    {
        if (!B2_IS_NULL(m_world))
        {
            b2DestroyWorld(m_world);
            m_world = b2_nullWorldId;
        }
    }

    PhysicsWorld::PhysicsWorld(PhysicsWorld&&) noexcept = default;
    PhysicsWorld& PhysicsWorld::operator=(PhysicsWorld&&) noexcept = default;

    void PhysicsWorld::SetGravity(eng::Vec2f gravity)
    {
        b2World_SetGravity(m_world, b2Vec2{ gravity.x, gravity.y });
    }

    void PhysicsWorld::Step(float dt, int subStepCount)
    {
        b2World_Step(m_world, dt, subStepCount);
    }

    static b2BodyId CreateBodyBoxPx(
        b2WorldId world,
        b2BodyType type,
        float cxPx, float cyPx, float wPx, float hPx,
        float density, float friction, float restitution)
    {
        // Body
        b2BodyDef bodyDef = b2DefaultBodyDef();
        bodyDef.type = type;
        bodyDef.position = b2Vec2{ ToMeters(cxPx), ToMeters(cyPx) };

        b2BodyId bodyId = b2CreateBody(world, &bodyDef);

        // Shape (box)
        b2Polygon poly = b2MakeBox(ToMeters(wPx * 0.5f), ToMeters(hPx * 0.5f));

        b2ShapeDef shapeDef = b2DefaultShapeDef();
        shapeDef.density = density;

        // Box2D v3.x material (most current vcpkg builds)
        shapeDef.material.friction = friction;
        shapeDef.material.restitution = restitution;

        b2CreatePolygonShape(bodyId, &shapeDef, &poly);

        return bodyId;
    }

    Body PhysicsWorld::CreateStaticBoxPx(float cxPx, float cyPx, float wPx, float hPx, float friction)
    {
        Body b;
        b.id = CreateBodyBoxPx(
            m_world, b2_staticBody,
            cxPx, cyPx, wPx, hPx,
            0.0f, friction, 0.0f);
        return b;
    }

    Body PhysicsWorld::CreateDynamicBoxPx(
        float cxPx, float cyPx, float wPx, float hPx,
        float density, float friction, float restitution)
    {
        Body b;
        b.id = CreateBodyBoxPx(
            m_world, b2_dynamicBody,
            cxPx, cyPx, wPx, hPx,
            density, friction, restitution);
        return b;
    }

    eng::Vec2f PhysicsWorld::GetPositionPx(Body b) const
    {
        b2Vec2 p = b2Body_GetPosition(b.id);
        return eng::Vec2f{ ToPixels(p.x), ToPixels(p.y) };
    }

    bool PhysicsWorld::IsValid(Body b) const
    {
        return b2Body_IsValid(b.id);
    }
}