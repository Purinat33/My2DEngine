#pragma once
#include <string>
#include <vector>

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

    struct Tileset
    {
        std::string texturePath; // atlas texture
        int tileWidth = 32;
        int tileHeight = 32;

        // Atlas layout
        int columns = 1;   // tiles per row in atlas
        int margin = 0;    // pixels
        int spacing = 0;   // pixels
    };

    struct TileLayer
    {
        std::string name = "Layer";
        int layer = 0; // render order shared with SpriteRendererComponent::layer
        bool visible = true;
        SDL_Color tint{ 255, 255, 255, 255 };

        // size = width*height, values are 0-based tile indices into the atlas, -1 = empty
        std::vector<int> tiles;
    };

    struct TilemapComponent
    {
        int width = 0;
        int height = 0;

        int tileWidth = 32;
        int tileHeight = 32;

        Tileset tileset;
        std::vector<TileLayer> layers;

        // Populated at runtime from window size for culling (kept here to avoid threading more params)
        int viewportW = 0;
        int viewportH = 0;
    };
}