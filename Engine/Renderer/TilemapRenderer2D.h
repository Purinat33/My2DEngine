#pragma once
#include "Renderer/Renderer2D.h"
#include "Assets/AssetManager.h"
#include "Scene/Components.h"

namespace my2d
{
    class TilemapRenderer2D
    {
    public:
        void DrawLayer(
            const TilemapComponent& tilemap,
            const TransformComponent& transform,
            const TileLayer& layer,
            AssetManager& assets,
            Renderer2D& renderer);
    };
}