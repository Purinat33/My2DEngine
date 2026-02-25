#pragma once
#include <string>
#include <vector>

#include "World/RoomGraph.h"
#include "Gameplay/TriggerWorld.h"
#include "Gameplay/GameState.h"
#include "Gameplay/CharacterController.h"

namespace eng::gameplay
{
    class MetroidvaniaWorld
    {
    public:
        void Bind(const eng::world::RoomGraph* graph, eng::gameplay::GameState* state)
        {
            m_graph = graph;
            m_state = state;
        }

        bool LoadRoom(const std::string& roomId, eng::gameplay::CharacterController& player, eng::Vec2f spawnPx)
        {
            if (!m_graph) return false;

            m_room = m_graph->Get(roomId);
            if (!m_room) return false;

            m_roomId = roomId;

            RebuildTriggersForRoom();
            player.TeleportPx(spawnPx);
            return true;
        }

        const eng::world::Tilemap& Map() const
        {
            // assumes LoadRoom succeeded
            return m_room->map;
        }

        const std::string& CurrentRoom() const { return m_roomId; }

        // Call after player.FixedUpdate each fixed step.
        void FixedUpdate(eng::gameplay::CharacterController& player, bool interactPressed)
        {
            if (!m_room) return;

            m_triggers.Update(player.GetBoundsPx(), m_events);

            // Pickups
            for (const auto& ev : m_events)
            {
                if (ev.type != TriggerEventType::Enter) continue;

                for (size_t i = 0; i < m_pickupTriggerIds.size(); ++i)
                {
                    const int tid = m_pickupTriggerIds[i];
                    if (tid <= 0 || tid != ev.triggerId) continue;

                    const auto& p = m_room->pickups[i];
                    if (m_state && !p.flag.empty() && m_state->HasFlag(p.flag))
                    {
                        m_triggers.SetEnabled(tid, false);
                        continue;
                    }

                    if (m_state)
                    {
                        if (!p.flag.empty()) m_state->SetFlag(p.flag);
                        if (p.grantAbilities != 0) m_state->GrantAbilityBits(p.grantAbilities);
                    }

                    m_triggers.SetEnabled(tid, false);
                }
            }

            // Doors
            for (const auto& ev : m_events)
            {
                for (size_t i = 0; i < m_doorTriggerIds.size(); ++i)
                {
                    const int tid = m_doorTriggerIds[i];
                    if (tid <= 0 || tid != ev.triggerId) continue;

                    const auto& d = m_room->doors[i];

                    const bool wantsUse =
                        (!d.requireInteract && ev.type == TriggerEventType::Enter) ||
                        (d.requireInteract && interactPressed && (ev.type == TriggerEventType::Enter || ev.type == TriggerEventType::Stay));

                    if (!wantsUse) continue;

                    if (m_state)
                    {
                        if (d.requiredAbilities != 0 && !m_state->HasAbilityBits(d.requiredAbilities))
                            continue;

                        if (!d.requiredFlag.empty() && !m_state->HasFlag(d.requiredFlag))
                            continue;
                    }
                    else
                    {
                        // no state bound => only allow doors with no requirements
                        if (d.requiredAbilities != 0 || !d.requiredFlag.empty())
                            continue;
                    }

                    // Transition
                    LoadRoom(d.targetRoom, player, d.targetSpawnPx);
                    return; // triggers rebuilt; stop processing this frame
                }
            }
        }

    private:
        void RebuildTriggersForRoom()
        {
            m_triggers.Clear();
            m_events.clear();

            m_doorTriggerIds.assign(m_room->doors.size(), 0);
            m_pickupTriggerIds.assign(m_room->pickups.size(), 0);

            for (size_t i = 0; i < m_room->doors.size(); ++i)
                m_doorTriggerIds[i] = m_triggers.AddTrigger(m_room->doors[i].boundsPx, /*kind=*/1);

            for (size_t i = 0; i < m_room->pickups.size(); ++i)
            {
                const auto& p = m_room->pickups[i];
                const int id = m_triggers.AddTrigger(p.boundsPx, /*kind=*/2);

                // If already collected, disable immediately.
                if (m_state && !p.flag.empty() && m_state->HasFlag(p.flag))
                    m_triggers.SetEnabled(id, false);

                m_pickupTriggerIds[i] = id;
            }
        }

        const eng::world::RoomGraph* m_graph = nullptr;
        eng::gameplay::GameState* m_state = nullptr;

        const eng::world::RoomDef* m_room = nullptr;
        std::string m_roomId;

        TriggerWorld m_triggers;
        std::vector<TriggerEvent> m_events;

        std::vector<int> m_doorTriggerIds;
        std::vector<int> m_pickupTriggerIds;
    };
}