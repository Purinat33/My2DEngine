#pragma once
#include "Physics/Box2D.h"

namespace my2d
{
    class PhysicsWorld
    {
    public:
        void Initialize(b2Vec2 gravity)
        {
            b2WorldDef def = b2DefaultWorldDef();
            def.gravity = gravity;
            m_worldId = b2CreateWorld(&def);
        }

        void Shutdown()
        {
            if (b2World_IsValid(m_worldId))
            {
                b2DestroyWorld(m_worldId);
                m_worldId = b2_nullWorldId;
            }
        }

        bool IsValid() const { return b2World_IsValid(m_worldId); }

        void Step(float fixedDt, int subSteps = 1)
        {
            if (!IsValid()) return;
            b2World_Step(m_worldId, fixedDt, subSteps);
        }

        b2WorldId WorldId() const { return m_worldId; }

    private:
        b2WorldId m_worldId = b2_nullWorldId;
    };
}