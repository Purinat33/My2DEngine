#include "pch.h"
#include "Gameplay/ProgressionSystem.h"
#include "Scene/Components.h"
#include "Physics/PhysicsSystem.h"
#include "Gameplay/GateSystem.h"
#include <vector>
#include <glm/geometric.hpp> // distance

namespace my2d
{
    void Progression_ApplyPersistence(Engine& engine, Scene& scene)
    {
        auto& ws = engine.GetWorldState();
        auto& reg = scene.Registry();

        std::vector<entt::entity> toDestroy;

        // 1) Persistent entities removed if a flag exists
        {
            auto view = reg.view<PersistentFlagComponent>();
            for (auto e : view)
            {
                auto& pf = view.get<PersistentFlagComponent>(e);
                if (!pf.flag.empty() && ws.HasFlag(pf.flag))
                    toDestroy.push_back(e);
            }
        }

        for (auto e : toDestroy)
        {
            Physics_DestroyRuntimeForEntity(scene, e);
            if (reg.valid(e))
                reg.destroy(e);
        }
    }

    void Progression_Update(Engine& engine, Scene& scene, Entity player)
    {
        if (!player) return;

        auto& reg = scene.Registry();
        if (!reg.valid(player.Handle())) return;

        auto& playerT = reg.get<TransformComponent>(player.Handle());

        bool grantedSomething = false;

        auto view = reg.view<TransformComponent, GrantProgressionComponent>();
        for (auto e : view)
        {
            auto& t = view.get<TransformComponent>(e);
            auto& g = view.get<GrantProgressionComponent>(e);

            const float dist = glm::distance(t.position, playerT.position);
            if (dist > g.radiusPx)
                continue;

            if (!g.setFlag.empty())
                engine.GetWorldState().SetFlag(g.setFlag);

            if (g.unlockAbility)
                engine.GetWorldState().UnlockAbility(g.ability);

            reg.destroy(e);
            grantedSomething = true;
            break;
        }

        if (grantedSomething)
        {
            Progression_ApplyPersistence(engine, scene);
            GateSystem_Update(engine, scene);
        }
    }
}