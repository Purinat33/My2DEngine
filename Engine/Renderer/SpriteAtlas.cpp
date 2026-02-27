#include "pch.h"
#include "Renderer/SpriteAtlas.h"
#include "Assets/AssetManager.h"
#include "Renderer/Texture2D.h"

#include <nlohmann/json.hpp>
#include <fstream>
#include <filesystem>
#include <spdlog/spdlog.h>

namespace my2d
{
    using json = nlohmann::json;

    static std::string JoinRelativeToFile(const std::string& filePath, const std::string& rel)
    {
        namespace fs = std::filesystem;
        fs::path base = fs::path(filePath).parent_path();
        fs::path p = fs::path(rel);
        if (p.is_absolute())
            return p.string();
        return (base / p).lexically_normal().string();
    }

    bool SpriteAtlas::LoadFromFile(AssetManager& assets, const std::string& atlasJsonPath)
    {
        m_regions.clear();
        m_path = atlasJsonPath;

        std::ifstream in(atlasJsonPath, std::ios::binary);
        if (!in)
        {
            spdlog::error("SpriteAtlas: failed to open '{}'", atlasJsonPath);
            return false;
        }

        json root;
        in >> root;

        // Optional default texture for true “packed atlas”
        std::string defaultTex;
        if (root.contains("texture") && root["texture"].is_string())
            defaultTex = JoinRelativeToFile(atlasJsonPath, root["texture"].get<std::string>());

        if (!root.contains("frames") || !root["frames"].is_object())
        {
            spdlog::error("SpriteAtlas: '{}' missing 'frames' object", atlasJsonPath);
            return false;
        }

        for (auto it = root["frames"].begin(); it != root["frames"].end(); ++it)
        {
            const std::string name = it.key();
            const json& jf = it.value();

            // Frame rect can be:
            // 1) {"x":..,"y":..,"w":..,"h":..,"texture":"optional.png"}
            // 2) {"frame":{"x":..,"y":..,"w":..,"h":..}, ...}  (TexturePacker-ish)
            SDL_Rect r{};

            const json* fr = &jf;
            if (jf.contains("frame") && jf["frame"].is_object())
                fr = &jf["frame"];

            r.x = fr->value("x", 0);
            r.y = fr->value("y", 0);
            r.w = fr->value("w", 0);
            r.h = fr->value("h", 0);

            std::string texPath = defaultTex;
            if (jf.contains("texture") && jf["texture"].is_string())
                texPath = JoinRelativeToFile(atlasJsonPath, jf["texture"].get<std::string>());

            if (texPath.empty())
            {
                spdlog::error("SpriteAtlas: frame '{}' has no texture (and no default texture)", name);
                continue;
            }

            auto tex = assets.GetTexture(texPath);
            if (!tex)
            {
                spdlog::error("SpriteAtlas: failed loading texture '{}' for frame '{}'", texPath, name);
                continue;
            }

            SpriteRegion region;
            region.texture = std::move(tex);
            region.rect = r;
            m_regions[name] = std::move(region);
        }

        return !m_regions.empty();
    }

    const SpriteRegion* SpriteAtlas::GetRegion(const std::string& name) const
    {
        auto it = m_regions.find(name);
        if (it == m_regions.end()) return nullptr;
        return &it->second;
    }
}