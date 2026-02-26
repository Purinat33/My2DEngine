#include "pch.h"
#include "Gameplay/GateSystem.h"

#include "Core/Engine.h"
#include "Scene/Scene.h"
#include "Scene/Components.h"
#include "Physics/PhysicsSystem.h"

#include <algorithm>
#include <vector>

namespace my2d
{
    static bool HasAllAbilities(const WorldState& ws, const std::vector<AbilityId>& list)
    {
        for (auto a : list)
            if (!ws.HasAbility(a)) return false;
        return true;
    }

    static bool HasAnyAbility(const WorldState& ws, const std::vector<AbilityId>& list)
    {
        if (list.empty()) return true;
        for (auto a : list)
            if (ws.HasAbility(a)) return true;
        return false;
    }

    static bool HasAllFlags(const WorldState& ws, const std::vector<std::string>& list)
    {
        for (const auto& f : list)
            if (!ws.HasFlag(f)) return false;
        return true;
    }

    static bool HasAnyFlag(const WorldState& ws, const std::vector<std::string>& list)
    {
        if (list.empty()) return true;
        for (const auto& f : list)
            if (ws.HasFlag(f)) return true;
        return false;
    }

    static bool EvaluateSatisfied(const WorldState& ws, const GateComponent& gate)
    {
        bool ok = true;

        // If all requirement lists are empty, consider satisfied by default.
        const bool anyReq =
            !gate.requireAllAbilities.empty() || !gate.requireAnyAbilities.empty() ||
            !gate.requireAllFlags.empty() || !gate.requireAnyFlags.empty();

        if (!anyReq)
            ok = true;
        else
        {
            ok = HasAllAbilities(ws, gate.requireAllAbilities) &&
                HasAnyAbility(ws, gate.requireAnyAbilities) &&
                HasAllFlags(ws, gate.requireAllFlags) &&
                HasAnyFlag(ws, gate.requireAnyFlags);
        }

        if (gate.invert)
            ok = !ok;

        return ok;
    }

    static void ApplyVisual(entt::registry& reg, entt::entity e, GateComponent& gate, bool open)
    {
        if (!reg.any_of<SpriteRendererComponent>(e))
            return;

        auto& spr = reg.get<SpriteRendererComponent>(e);

        if (gate.overrideTint)
            spr.tint = open ? gate.openTint : gate.closedTint;

        if (gate.hideWhenOpen && open)
            spr.tint.a = 0;
        else if (gate.hideWhenOpen && !open)
            spr.tint.a = 255;
    }

    void GateSystem_Update(Engine& engine, Scene& scene)
    {
        auto& ws = engine.GetWorldState();
        auto& reg = scene.Registry();

        std::vector<entt::entity> toDestroy;

        auto view = reg.view<GateComponent>();
        for (auto e : view)
        {
            auto& gate = view.get<GateComponent>(e);

            const bool satisfied = EvaluateSatisfied(ws, gate);
            const bool shouldOpen = gate.openWhenSatisfied ? satisfied : !satisfied;

            if (!gate.initialized)
            {
                gate.initialized = true;
                gate.isOpen = shouldOpen;
            }

            if (shouldOpen == gate.isOpen)
            {
                // Still ensure visuals match (useful if artist edits tint)
                ApplyVisual(reg, e, gate, gate.isOpen);
                continue;
            }

            // State changed
            gate.isOpen = shouldOpen;

            if (gate.isOpen)
            {
                if (gate.openBehavior == GateOpenBehavior::DestroyEntity)
                {
                    Physics_DestroyRuntimeForEntity(scene, e);
                    toDestroy.push_back(e);
                    continue;
                }

                // Open: disable collision, optionally hide
                if (reg.any_of<RigidBody2DComponent, BoxCollider2DComponent>(e))
                {
                    auto& rb = reg.get<RigidBody2DComponent>(e);
                    auto& bc = reg.get<BoxCollider2DComponent>(e);
                    rb.enabled = false;
                    bc.enabled = false;
                    Physics_DestroyRuntimeForEntity(scene, e);
                }

                ApplyVisual(reg, e, gate, true);
            }
            else
            {
                // Closed: enable collision (Physics_CreateRuntime will recreate body next tick)
                if (reg.any_of<RigidBody2DComponent, BoxCollider2DComponent>(e))
                {
                    reg.get<RigidBody2DComponent>(e).enabled = true;
                    reg.get<BoxCollider2DComponent>(e).enabled = true;
                }

                ApplyVisual(reg, e, gate, false);
            }
        }

        for (auto e : toDestroy)
            if (reg.valid(e)) reg.destroy(e);

        // Ensure newly-closed gates can get bodies immediately (useful after toggles)
        Physics_CreateRuntime(scene, engine.GetPhysics(), engine.PixelsPerMeter());
    }
}