#pragma once
#include <string>
#include <unordered_map>
#include <vector>

namespace my2d
{
    struct AnimationClip
    {
        std::vector<std::string> frames; // region names in atlas
        float fps = 10.0f;
        bool loop = true;
    };

    class AnimationSet
    {
    public:
        bool LoadFromFile(const std::string& path);

        const std::string& AtlasPath() const { return m_atlasPath; }
        const AnimationClip* GetClip(const std::string& name) const;

    private:
        std::string m_atlasPath;
        std::unordered_map<std::string, AnimationClip> m_clips;
    };
}