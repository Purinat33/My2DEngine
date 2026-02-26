#pragma once
#include <cstdint>
#include <string>
#include <unordered_set>
#include <vector>

#include "Gameplay/Ability.h"

namespace my2d
{
    class WorldState
    {
    public:
        // ---- Flags ----
        bool HasFlag(const std::string& flag) const;
        void SetFlag(std::string flag);
        void ClearFlag(const std::string& flag);

        // ---- Abilities ----
        bool HasAbility(AbilityId a) const;
        void UnlockAbility(AbilityId a);
        void LockAbility(AbilityId a);

        // Debug/serialization helpers
        const std::unordered_set<std::string>& Flags() const { return m_flags; }
        uint64_t AbilitiesMask() const { return m_abilities; }
        void SetAbilitiesMask(uint64_t mask) { m_abilities = mask; }

    private:
        std::unordered_set<std::string> m_flags;
        uint64_t m_abilities = 0;
    };
}