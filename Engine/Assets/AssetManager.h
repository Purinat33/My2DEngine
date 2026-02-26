#pragma once
#include <memory>
#include <string>
#include <unordered_map>

#include "Platform/Sdl.h"

namespace my2d
{
    class Texture2D;

    class AssetManager
    {
    public:
        void SetRenderer(SDL_Renderer* renderer) { m_renderer = renderer; }
        void SetContentRoot(std::string root) { m_contentRoot = std::move(root); }

        std::shared_ptr<Texture2D> GetTexture(const std::string& path);

        void Clear();

    private:
        std::string ResolvePath(const std::string& path) const;

    private:
        SDL_Renderer* m_renderer = nullptr;
        std::string m_contentRoot;

        std::unordered_map<std::string, std::shared_ptr<Texture2D>> m_textureCache;
    };
}