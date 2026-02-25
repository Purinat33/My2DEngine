#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <utility>

#include "Core/Types.h"
#include "World/Tilemap.h"

namespace eng::world
{
    struct DoorDef
    {
        eng::RectF boundsPx{};
        std::string targetRoom;
        eng::Vec2f targetSpawnPx{ 0,0 };

        uint32_t requiredAbilities = 0;  // bitmask (use gameplay::Ability values)
        std::string requiredFlag;        // optional flag requirement
        bool requireInteract = true;     // metroidvania-style doors
    };

    struct PickupDef
    {
        eng::RectF boundsPx{};
        std::string flag;                // e.g. "pickup_dash"
        uint32_t grantAbilities = 0;      // bitmask (use gameplay::Ability values)
    };

    struct RoomDef
    {
        std::string id;
        Tilemap map;
        eng::Vec2f defaultSpawnPx{ 0,0 };
        std::vector<DoorDef> doors;
        std::vector<PickupDef> pickups;
    };

    class RoomGraph
    {
    public:
        void Reserve(size_t n) { m_rooms.reserve(n); }

        void AddRoom(RoomDef room)
        {
            const std::string key = room.id;
            m_rooms.emplace(key, std::move(room));
        }

        const RoomDef* Get(const std::string& id) const
        {
            auto it = m_rooms.find(id);
            if (it == m_rooms.end()) return nullptr;
            return &it->second;
        }

    private:
        std::unordered_map<std::string, RoomDef> m_rooms;
    };
}