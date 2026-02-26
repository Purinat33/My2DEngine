#pragma once
#include <string>

#include <glm/vec2.hpp>
#include "Platform/Sdl.h"

namespace my2d
{
    struct TagComponent
    {
        std::string tag;
    };

    struct TransformComponent
    {
        glm::vec2 position{ 0.0f, 0.0f };  // world units (treat as pixels for now)
        float rotationDeg = 0.0f;
        glm::vec2 scale{ 1.0f, 1.0f };
    };

    struct SpriteRendererComponent
    {
        std::string texturePath;          // asset key/path
        glm::vec2 size{ 64.0f, 64.0f };   // world size (pixels)
        SDL_Color tint{ 255, 255, 255, 255 };
        int layer = 0;

        bool useSourceRect = false;
        SDL_Rect sourceRect{ 0, 0, 0, 0 };

        SDL_RendererFlip flip = SDL_FLIP_NONE;
    };
}