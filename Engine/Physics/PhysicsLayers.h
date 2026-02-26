#pragma once
#include <cstdint>

namespace my2d::PhysicsLayers
{
    static constexpr uint64_t Environment = 1ull << 0;
    static constexpr uint64_t Player = 1ull << 1;
    static constexpr uint64_t Enemy = 1ull << 2;
    static constexpr uint64_t Hitbox = 1ull << 3;

    static constexpr uint64_t All = 0xFFFFFFFFFFFFFFFFull;
}