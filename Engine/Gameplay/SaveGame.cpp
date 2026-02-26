#include "pch.h"
#include "Gameplay/SaveGame.h"
#include "Gameplay/Ability.h"

#include <nlohmann/json.hpp>
#include <fstream>

namespace my2d
{
    using json = nlohmann::json;

    static json AbilitiesToJson(uint64_t mask)
    {
        json arr = json::array();
        for (uint8_t i = 0; i < (uint8_t)AbilityId::COUNT; ++i)
        {
            AbilityId a = (AbilityId)i;
            if (mask & AbilityBit(a))
                arr.push_back(std::string(AbilityName(a)));
        }
        return arr;
    }

    static uint64_t AbilitiesFromJson(const json& j)
    {
        uint64_t mask = 0;
        if (j.is_array())
        {
            for (auto& v : j)
            {
                AbilityId a;
                if (v.is_string() && AbilityFromName(v.get<std::string>(), a))
                    mask |= AbilityBit(a);
            }
        }
        else if (j.is_number_unsigned())
        {
            mask = j.get<uint64_t>();
        }
        return mask;
    }

    bool SaveGame::SaveToFile(const std::string& path) const
    {
        json j;
        j["version"] = version;
        j["flags"] = json::array();
        for (const auto& f : world.Flags())
            j["flags"].push_back(f);

        j["abilities"] = AbilitiesToJson(world.AbilitiesMask());

        std::ofstream out(path, std::ios::binary);
        if (!out) return false;
        out << j.dump(2);
        return true;
    }

    bool SaveGame::LoadFromFile(const std::string& path, SaveGame& outSave)
    {
        std::ifstream in(path, std::ios::binary);
        if (!in) return false;

        json j;
        in >> j;

        outSave.version = j.value("version", 1);

        WorldState ws;
        if (j.contains("flags") && j["flags"].is_array())
        {
            for (auto& v : j["flags"])
                if (v.is_string()) ws.SetFlag(v.get<std::string>());
        }

        if (j.contains("abilities"))
            ws.SetAbilitiesMask(AbilitiesFromJson(j["abilities"]));

        outSave.world = std::move(ws);
        return true;
    }
}