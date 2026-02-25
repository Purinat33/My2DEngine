#pragma once
#include <cstdint>
#include <string>
#include <unordered_set>
#include <fstream>
#include <nlohmann/json.hpp>

#include "Core/Types.h"

namespace eng::gameplay
{
    enum class Ability : uint32_t
    {
        Dash = 1u << 0,
        DoubleJump = 1u << 1,
        WallClimb = 1u << 2,
        Swim = 1u << 3,
    };

    class GameState
    {
    public:
        // ---- abilities ----
        bool HasAbility(Ability a) const { return (m_abilities & (uint32_t)a) != 0; }
        bool HasAbilityBits(uint32_t bits) const { return (m_abilities & bits) == bits; }

        void GrantAbility(Ability a) { m_abilities |= (uint32_t)a; }
        void GrantAbilityBits(uint32_t bits) { m_abilities |= bits; }

        uint32_t AbilityBits() const { return m_abilities; }

        // ---- flags (doors opened, pickups collected, bosses defeated...) ----
        bool HasFlag(const std::string& key) const { return m_flags.find(key) != m_flags.end(); }
        void SetFlag(const std::string& key) { m_flags.insert(key); }
        void ClearFlag(const std::string& key) { m_flags.erase(key); }

        // ---- checkpoint (optional but very metroidvania) ----
        void SetCheckpoint(const std::string& roomId, eng::Vec2f posPx)
        {
            m_hasCheckpoint = true;
            m_checkpointRoom = roomId;
            m_checkpointPosPx = posPx;
        }

        bool HasCheckpoint() const { return m_hasCheckpoint; }
        const std::string& CheckpointRoom() const { return m_checkpointRoom; }
        eng::Vec2f CheckpointPosPx() const { return m_checkpointPosPx; }

        void ClearAll()
        {
            m_abilities = 0;
            m_flags.clear();
            m_hasCheckpoint = false;
            m_checkpointRoom.clear();
            m_checkpointPosPx = { 0,0 };
        }

        bool SaveToFile(const std::string& path) const
        {
            nlohmann::json j;
            j["version"] = 1;
            j["abilities"] = m_abilities;

            j["flags"] = nlohmann::json::array();
            for (const auto& f : m_flags) j["flags"].push_back(f);

            nlohmann::json cp;
            cp["has"] = m_hasCheckpoint;
            cp["room"] = m_checkpointRoom;
            cp["pos"] = { m_checkpointPosPx.x, m_checkpointPosPx.y };
            j["checkpoint"] = cp;

            std::ofstream out(path, std::ios::binary);
            if (!out) return false;
            out << j.dump(2);
            return true;
        }

        bool LoadFromFile(const std::string& path)
        {
            std::ifstream in(path, std::ios::binary);
            if (!in) return false;

            nlohmann::json j;
            in >> j;

            const int version = j.value("version", 0);
            if (version != 1) return false;

            m_abilities = j.value("abilities", 0u);

            m_flags.clear();
            if (j.contains("flags") && j["flags"].is_array())
            {
                for (const auto& it : j["flags"])
                    if (it.is_string()) m_flags.insert(it.get<std::string>());
            }

            m_hasCheckpoint = false;
            m_checkpointRoom.clear();
            m_checkpointPosPx = { 0,0 };

            if (j.contains("checkpoint") && j["checkpoint"].is_object())
            {
                const auto& cp = j["checkpoint"];
                m_hasCheckpoint = cp.value("has", false);
                m_checkpointRoom = cp.value("room", std::string{});

                if (cp.contains("pos") && cp["pos"].is_array() && cp["pos"].size() == 2)
                {
                    m_checkpointPosPx.x = cp["pos"][0].get<float>();
                    m_checkpointPosPx.y = cp["pos"][1].get<float>();
                }
            }

            return true;
        }

    private:
        uint32_t m_abilities = 0;
        std::unordered_set<std::string> m_flags;

        bool m_hasCheckpoint = false;
        std::string m_checkpointRoom;
        eng::Vec2f m_checkpointPosPx{ 0,0 };
    };
}