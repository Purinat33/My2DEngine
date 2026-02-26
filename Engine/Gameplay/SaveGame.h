#pragma once
#include <string>
#include "Gameplay/WorldState.h"

namespace my2d
{
    struct SaveGame
    {
        int version = 1;
        WorldState world;

        bool SaveToFile(const std::string& path) const;
        static bool LoadFromFile(const std::string& path, SaveGame& out);
    };
}