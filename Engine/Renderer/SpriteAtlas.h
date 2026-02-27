#pragma once
#include <string>
#include <unordered_map>
#include <memory>

#include "Platform/Sdl.h"

namespace my2d
{
    class Texture2D;
    class AssetManager;

    struct SpriteRegion
    {
        std::shared_ptr<Texture2D> texture;
        SDL_Rect rect{ 0,0,0,0 };
    };

    class SpriteAtlas
    {
    public:
        bool LoadFromFile(AssetManager& assets, const std::string& atlasJsonPath);

        const SpriteRegion* GetRegion(const std::string& name) const;

        const std::string& Path() const { return m_path; }

    private:
        std::string m_path;
        std::unordered_map<std::string, SpriteRegion> m_regions;
    };
}