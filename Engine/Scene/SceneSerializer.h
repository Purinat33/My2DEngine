#pragma once
#include <string>

namespace my2d
{
    class Scene;

    class SceneSerializer
    {
    public:
        static bool SaveToFile(const Scene& scene, const std::string& path);
        static bool LoadFromFile(Scene& scene, const std::string& path);
    };
}