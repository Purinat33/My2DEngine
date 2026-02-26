#include "pch.h"
#include "Assets/AssetManager.h"
#include "Renderer/Texture2D.h"

#include <filesystem>
#include <spdlog/spdlog.h>

namespace my2d
{
    std::string AssetManager::ResolvePath(const std::string& path) const
    {
        namespace fs = std::filesystem;

        fs::path p(path);

        if (p.is_absolute())
            return p.string();

        if (!m_contentRoot.empty())
        {
            fs::path root(m_contentRoot);
            return (root / p).lexically_normal().string();
        }

        return p.lexically_normal().string();
    }

    std::shared_ptr<Texture2D> AssetManager::GetTexture(const std::string& path)
    {
        if (!m_renderer)
        {
            spdlog::error("AssetManager::GetTexture called before SetRenderer()");
            return {};
        }

        const std::string resolved = ResolvePath(path);

        // Cache hit?
        if (auto it = m_textureCache.find(resolved); it != m_textureCache.end())
        {
            if (auto existing = it->second.lock())
                return existing;
        }

        auto tex = std::make_shared<Texture2D>();
        if (!tex->LoadFromFile(m_renderer, resolved))
            return {};

        m_textureCache[resolved] = tex;
        return tex;
    }

    void AssetManager::Clear()
    {
        m_textureCache.clear();
    }
}