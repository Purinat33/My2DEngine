#pragma once
#include <cstdint>
#include <string_view>

namespace my2d
{
    // Keep under 64 entries if you use uint64_t mask
    enum class AbilityId : uint8_t
    {
        Dash = 0,
        DoubleJump,
        WallClimb,
        Swim,
        COUNT
    };

    constexpr uint64_t AbilityBit(AbilityId a)
    {
        return 1ull << static_cast<uint8_t>(a);
    }

    inline std::string_view AbilityName(AbilityId a)
    {
        switch (a)
        {
        case AbilityId::Dash: return "Dash";
        case AbilityId::DoubleJump: return "DoubleJump";
        case AbilityId::WallClimb: return "WallClimb";
        case AbilityId::Swim: return "Swim";
        default: return "Unknown";
        }
    }

    inline bool AbilityFromName(std::string_view s, AbilityId& out)
    {
        if (s == "Dash") { out = AbilityId::Dash; return true; }
        if (s == "DoubleJump") { out = AbilityId::DoubleJump; return true; }
        if (s == "WallClimb") { out = AbilityId::WallClimb; return true; }
        if (s == "Swim") { out = AbilityId::Swim; return true; }
        return false;
    }
}