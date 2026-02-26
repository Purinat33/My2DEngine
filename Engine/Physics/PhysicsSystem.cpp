#include "pch.h"
#include "Physics/PhysicsSystem.h"
#include "Scene/Components.h"
#include "Physics/PhysicsLayers.h"

#include <algorithm>

namespace my2d
{
    static b2BodyType ToB2Type(BodyType2D t)
    {
        switch (t)
        {
        case BodyType2D::Static: return b2_staticBody;
        case BodyType2D::Kinematic: return b2_kinematicBody;
        case BodyType2D::Dynamic: return b2_dynamicBody;
        default: return b2_dynamicBody;
        }
    }

    static float DegToRad(float deg) { return deg * 0.01745329251994329577f; }
    static float RadToDeg(float rad) { return rad * 57.295779513082320876f; }

    void Physics_CreateRuntime(Scene& scene, PhysicsWorld& physics, float ppm)
    {
        if (!physics.IsValid()) return;

        auto& reg = scene.Registry();
        auto view = reg.view<TransformComponent, RigidBody2DComponent, BoxCollider2DComponent>();

        for (auto e : view)
        {
            auto& tc = view.get<TransformComponent>(e);
            auto& rb = view.get<RigidBody2DComponent>(e);
            auto& bc = view.get<BoxCollider2DComponent>(e);

            if (b2Body_IsValid(rb.bodyId))
                continue;

            // Treat Transform.position as TOP-LEFT in pixels.
            // Box2D body position is the body origin (we'll use center of collider).
            const glm::vec2 half = bc.size * 0.5f;
            const glm::vec2 centerPx = tc.position + bc.offset + half;

            b2BodyDef bd = b2DefaultBodyDef();
            bd.type = ToB2Type(rb.type);
            bd.position = b2Vec2{ centerPx.x / ppm, centerPx.y / ppm };
            bd.rotation = b2MakeRot(DegToRad(tc.rotationDeg));

            bd.fixedRotation = rb.fixedRotation;
            bd.enableSleep = rb.enableSleep;
            bd.isBullet = rb.isBullet;
            bd.gravityScale = rb.gravityScale;
            bd.linearDamping = rb.linearDamping;
            bd.angularDamping = rb.angularDamping;

            rb.bodyId = b2CreateBody(physics.WorldId(), &bd);

            b2ShapeDef sd = b2DefaultShapeDef();
            sd.density = bc.density;
            sd.material.friction = bc.friction;
            sd.material.restitution = bc.restitution;
            sd.isSensor = bc.isSensor;
            sd.filter.categoryBits = my2d::PhysicsLayers::Player;
            sd.filter.maskBits = my2d::PhysicsLayers::Environment;

            const float halfW = (bc.size.x * 0.5f) / ppm;
            const float halfH = (bc.size.y * 0.5f) / ppm;

            // Box shape centered on body origin (no local offset because we baked it into bd.position)
            b2Polygon box = b2MakeBox(halfW, halfH);
            bc.shapeId = b2CreatePolygonShape(rb.bodyId, &sd, &box);
        }
    }

    void Physics_SyncTransforms(Scene& scene, float ppm)
    {
        auto& reg = scene.Registry();
        auto view = reg.view<TransformComponent, RigidBody2DComponent, BoxCollider2DComponent>();

        for (auto e : view)
        {
            auto& tc = view.get<TransformComponent>(e);
            auto& rb = view.get<RigidBody2DComponent>(e);
            auto& bc = view.get<BoxCollider2DComponent>(e);

            if (!b2Body_IsValid(rb.bodyId))
                continue;

            const b2Vec2 pos = b2Body_GetPosition(rb.bodyId);
            const b2Rot rot = b2Body_GetRotation(rb.bodyId);

            // Convert body center back to TOP-LEFT pixels
            const glm::vec2 centerPx{ pos.x * ppm, pos.y * ppm };
            const glm::vec2 half = bc.size * 0.5f;

            tc.position = centerPx - bc.offset - half;

            // Rotation (optional if fixedRotation is true, but harmless)
            const float angleRad = std::atan2(rot.s, rot.c);
            tc.rotationDeg = RadToDeg(angleRad);
        }
    }
}