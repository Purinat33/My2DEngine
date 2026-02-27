#include "pch.h"
#include "Assets/AssetManager.h"
#include "Renderer/Texture2D.h"
#include "Renderer/SpriteAtlas.h"
#include "Renderer/AnimationSet.h"
#include <cctype>
#include <filesystem>
#include <spdlog/spdlog.h>

namespace my2d
{
    std::string AssetManager::ResolvePath(const std::string& path) const
    {
        namespace fs = std::filesystem;

        fs::path p(path);

        // absolute path? return normalized
        if (p.is_absolute() || p.has_root_name())
            return p.lexically_normal().string();

        fs::path root(m_contentRoot);
        fs::path normP = p.lexically_normal();

        // If caller already passed something starting with contentRoot, don't prefix again.
        // Compare using generic_string to avoid slash differences.
        const std::string rootS = root.lexically_normal().generic_string();
        const std::string pS = normP.generic_string();

        auto toLower = [](std::string s) {
            for (char& c : s) c = (char)std::tolower((unsigned char)c);
            return s;
            };

        const std::string rootL = toLower(rootS);
        const std::string pL = toLower(pS);

        if (pL == rootL || (pL.size() > rootL.size() && pL.rfind(rootL + "/", 0) == 0))
            return normP.lexically_normal().string();

        return (root / normP).lexically_normal().string();
    }


    void AssetManager::SetContentRoot(const std::string& root)
    {
        namespace fs = std::filesystem;

        fs::path rel(root);

        // If absolute already, keep it
        fs::path resolved;
        if (rel.is_absolute() || rel.has_root_name())
        {
            resolved = rel;
        }
        else
        {
            // Search upward from current working directory
            fs::path base = fs::current_path();
            bool found = false;

            for (int i = 0; i < 10; ++i)
            {
                fs::path cand = (base / rel).lexically_normal();
                if (fs::exists(cand))
                {
                    resolved = cand;
                    found = true;
                    break;
                }
                if (!base.has_parent_path())
                    break;
                base = base.parent_path();
            }

            if (!found)
            {
                // Fallback: absolute from current working dir
                resolved = fs::absolute(rel).lexically_normal();
            }
        }

        m_contentRoot = resolved.lexically_normal().string();
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
            return it->second;

        auto tex = std::make_shared<Texture2D>();
        if (!tex->LoadFromFile(m_renderer, resolved))
            return {};

        m_textureCache.emplace(resolved, tex);
        return tex;
    }

    std::shared_ptr<AnimationSet> AssetManager::GetAnimationSet(const std::string& animSetPath)
    {
        const std::string resolved = ResolvePath(animSetPath);

        if (auto it = m_animSetCache.find(resolved); it != m_animSetCache.end())
            return it->second;

        auto set = std::make_shared<AnimationSet>();
        if (!set->LoadFromFile(resolved))
            return {};

        m_animSetCache.emplace(resolved, set);
        return set;
    }

    std::shared_ptr<SpriteAtlas> AssetManager::GetAtlas(const std::string& atlasJsonPath)
    {
        const std::string resolved = ResolvePath(atlasJsonPath);

        if (auto it = m_atlasCache.find(resolved); it != m_atlasCache.end())
            return it->second; // may be nullptr (failed cached)

        auto atlas = std::make_shared<SpriteAtlas>();
        if (!atlas->LoadFromFile(*this, resolved))
        {
            m_atlasCache.emplace(resolved, nullptr); // cache failure
            return {};
        }

        m_atlasCache.emplace(resolved, atlas);
        return atlas;
    }

    void AssetManager::Clear()
    {
        m_textureCache.clear();
        m_atlasCache.clear();
        m_animSetCache.clear();
    }
}