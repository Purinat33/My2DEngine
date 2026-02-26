#include "pch.h"
#include "Gameplay/ProgressionSystem.h"
#include "Scene/Components.h"

#include <glm/geometric.hpp> // distance

namespace my2d
{
    void Progression_ApplyPersistence(Engine& engine, Scene& scene)
    {
        auto& ws = engine.GetWorldState();
        auto& reg = scene.Registry();

        auto view = reg.view<PersistentFlagComponent>();
        for (auto e : view)
        {
            auto& pf = view.get<PersistentFlagComponent>(e);
            if (!pf.flag.empty() && ws.HasFlag(pf.flag))
            {
                reg.destroy(e);
            }
        }
    }

    void Progression_Update(Engine& engine, Scene& scene, Entity player)
    {
        if (!player) return;

        auto& reg = scene.Registry();
        if (!reg.valid(player.Handle())) return;

        auto& playerT = reg.get<TransformComponent>(player.Handle());

        auto view = reg.view<TransformComponent, GrantProgressionComponent>();
        for (auto e : view)
        {
            auto& t = view.get<TransformComponent>(e);
            auto& g = view.get<GrantProgressionComponent>(e);

            const float dist = glm::distance(t.position, playerT.position);
            if (dist > g.radiusPx)
                continue;

            // grant
            if (!g.setFlag.empty())
                engine.GetWorldState().SetFlag(g.setFlag);

            if (g.unlockAbility)
                engine.GetWorldState().UnlockAbility(g.ability);

            // remove pickup
            reg.destroy(e);
            break; // avoid iterator invalidation issues + multiple pickups in one tick
        }
    }
}