#include "pch.h"
#include "Gameplay/WorldState.h"

namespace my2d
{
    bool WorldState::HasFlag(const std::string& flag) const
    {
        return m_flags.find(flag) != m_flags.end();
    }

    void WorldState::SetFlag(std::string flag)
    {
        m_flags.insert(std::move(flag));
    }

    void WorldState::ClearFlag(const std::string& flag)
    {
        m_flags.erase(flag);
    }

    bool WorldState::HasAbility(AbilityId a) const
    {
        return (m_abilities & AbilityBit(a)) != 0;
    }

    void WorldState::UnlockAbility(AbilityId a)
    {
        m_abilities |= AbilityBit(a);
    }

    void WorldState::LockAbility(AbilityId a)
    {
        m_abilities &= ~AbilityBit(a);
    }
}