#pragma once
#include <memory>
#include <string>
#include <unordered_map>

#include "Platform/Sdl.h"

namespace my2d
{
    class Texture2D;
    class SpriteAtlas;
    class AnimationSet;
    class AssetManager
    {
    public:
        void SetRenderer(SDL_Renderer* renderer) { m_renderer = renderer; }
        void SetContentRoot(const std::string& root); 

        std::shared_ptr<Texture2D> GetTexture(const std::string& path);

        void Clear();
        std::shared_ptr<SpriteAtlas> GetAtlas(const std::string& atlasJsonPath);
        std::shared_ptr<AnimationSet> GetAnimationSet(const std::string& animSetPath);

    private:
        std::string ResolvePath(const std::string& path) const;

    private:
        SDL_Renderer* m_renderer = nullptr;
        std::string m_contentRoot;

        std::unordered_map<std::string, std::shared_ptr<Texture2D>> m_textureCache;
        std::unordered_map<std::string, std::shared_ptr<SpriteAtlas>> m_atlasCache;
        std::unordered_map<std::string, std::shared_ptr<AnimationSet>> m_animSetCache;
    };
}