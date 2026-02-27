#include "pch.h"
#include "Renderer/AnimationSet.h"

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

    bool AnimationSet::LoadFromFile(const std::string& path)
    {
        m_clips.clear();
        m_atlasPath.clear();

        std::ifstream in(path, std::ios::binary);
        if (!in)
        {
            spdlog::error("AnimationSet: failed to open '{}'", path);
            return false;
        }

        json root;
        in >> root;

        if (!root.contains("atlas") || !root["atlas"].is_string())
        {
            spdlog::error("AnimationSet: '{}' missing 'atlas' string", path);
            return false;
        }

        m_atlasPath = JoinRelativeToFile(path, root["atlas"].get<std::string>());

        if (!root.contains("clips") || !root["clips"].is_object())
        {
            spdlog::error("AnimationSet: '{}' missing 'clips' object", path);
            return false;
        }

        for (auto it = root["clips"].begin(); it != root["clips"].end(); ++it)
        {
            const std::string clipName = it.key();
            const json& jc = it.value();

            AnimationClip c;
            c.fps = (float)jc.value("fps", 10.0);
            c.loop = jc.value("loop", true);

            if (jc.contains("frames") && jc["frames"].is_array())
                c.frames = jc["frames"].get<std::vector<std::string>>();

            if (c.frames.empty())
            {
                spdlog::warn("AnimationSet: clip '{}' has no frames", clipName);
                continue;
            }

            m_clips.emplace(clipName, std::move(c));
        }

        return !m_clips.empty();
    }

    const AnimationClip* AnimationSet::GetClip(const std::string& name) const
    {
        auto it = m_clips.find(name);
        if (it == m_clips.end()) return nullptr;
        return &it->second;
    }
}