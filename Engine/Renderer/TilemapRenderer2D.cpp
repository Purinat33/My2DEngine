#include "pch.h"
#include "Renderer/TilemapRenderer2D.h"
#include "Renderer/Texture2D.h"

#include <algorithm>

namespace my2d
{
    static SDL_Rect CalcTileSrcRect(const Tileset& ts, int tileIndex)
    {
        // tileIndex is 0-based into the atlas (NOT a global id)
        const int cols = std::max(1, ts.columns);
        const int tx = tileIndex % cols;
        const int ty = tileIndex / cols;

        SDL_Rect r{};
        r.x = ts.margin + tx * (ts.tileWidth + ts.spacing);
        r.y = ts.margin + ty * (ts.tileHeight + ts.spacing);
        r.w = ts.tileWidth;
        r.h = ts.tileHeight;
        return r;
    }

    void TilemapRenderer2D::DrawLayer(
        const TilemapComponent& tilemap,
        const TransformComponent& transform,
        const TileLayer& layer,
        AssetManager& assets,
        Renderer2D& renderer)
    {
        if (!layer.visible) return;

        if (tilemap.width <= 0 || tilemap.height <= 0)
        {
            spdlog::warn("Tilemap '{}' has invalid size {}x{}", layer.name, tilemap.width, tilemap.height);
            return;
        }

        if (tilemap.tileWidth <= 0 || tilemap.tileHeight <= 0)
        {
            spdlog::warn("Tilemap '{}' has invalid tile size {}x{}", layer.name, tilemap.tileWidth, tilemap.tileHeight);
            return;
        }

        const int expected = tilemap.width * tilemap.height;
        if ((int)layer.tiles.size() != expected)
        {
            spdlog::error("Tile layer '{}' has {} tiles but expected {}", layer.name, (int)layer.tiles.size(), expected);
            return;
        }

        if (tilemap.tileset.texturePath.empty())
        {
            spdlog::error("Tilemap layer '{}' tileset.texturePath is empty", layer.name);
            return;
        }

        auto tex = assets.GetTexture(tilemap.tileset.texturePath);
        if (!tex)
        {
            spdlog::error("Tilemap layer '{}' failed to load tileset texture '{}'", layer.name, tilemap.tileset.texturePath);
            return;
        }

        const auto& cam = renderer.GetCamera();

        const float halfW = cam.ViewportW() * 0.5f / cam.Zoom();
        const float halfH = cam.ViewportH() * 0.5f / cam.Zoom();

        const glm::vec2 camPos = cam.Position();
        const glm::vec2 worldMin = camPos - glm::vec2(halfW, halfH);
        const glm::vec2 worldMax = camPos + glm::vec2(halfW, halfH);

        const glm::vec2 origin = transform.position;

        const int minX = std::max(0, (int)std::floor((worldMin.x - origin.x) / (float)tilemap.tileWidth) - 1);
        const int minY = std::max(0, (int)std::floor((worldMin.y - origin.y) / (float)tilemap.tileHeight) - 1);
        const int maxX = std::min(tilemap.width - 1, (int)std::floor((worldMax.x - origin.x) / (float)tilemap.tileWidth) + 1);
        const int maxY = std::min(tilemap.height - 1, (int)std::floor((worldMax.y - origin.y) / (float)tilemap.tileHeight) + 1);

        // Heuristic: if the layer contains 0s, assume Tiled-style (0 empty, 1-based gids)
        const bool zeroMeansEmpty =
            std::find(layer.tiles.begin(), layer.tiles.end(), 0) != layer.tiles.end();

        // Tiled flip flags
        constexpr uint32_t FLIP_H = 0x80000000u;
        constexpr uint32_t FLIP_V = 0x40000000u;
        constexpr uint32_t FLIP_D = 0x20000000u;

        bool warnedDiagonal = false;
        bool warnedOutOfBounds = false;

        for (int y = minY; y <= maxY; ++y)
        {
            for (int x = minX; x <= maxX; ++x)
            {
                const int idx = y * tilemap.width + x;
                const int raw = layer.tiles[idx];

                // Engine-native empty
                if (raw == -1) continue;

                uint32_t gid = (uint32_t)raw;

                const bool hasFlags = (gid & (FLIP_H | FLIP_V | FLIP_D)) != 0;
                const bool treatAsTiled = zeroMeansEmpty || hasFlags;

                int tileIndex = -1;
                SDL_RendererFlip flip = SDL_FLIP_NONE;
                float rotationDeg = 0.0f;

                if (treatAsTiled)
                {
                    const bool fh = (gid & FLIP_H) != 0;
                    const bool fv = (gid & FLIP_V) != 0;
                    const bool fd = (gid & FLIP_D) != 0;

                    gid &= ~(FLIP_H | FLIP_V | FLIP_D);

                    // Tiled empty
                    if (gid == 0) continue;

                    // Single-tileset assumption: firstgid = 1
                    tileIndex = (int)gid - 1;

                    if (fh) flip = (SDL_RendererFlip)(flip | SDL_FLIP_HORIZONTAL);
                    if (fv) flip = (SDL_RendererFlip)(flip | SDL_FLIP_VERTICAL);

                    // Diagonal flip needs rotation+flip mapping; warn but still draw “unrotated”
                    if (fd && !warnedDiagonal)
                    {
                        warnedDiagonal = true;
                        spdlog::warn("Tile layer '{}' contains diagonal-flipped tiles (Tiled FLIP_D). They will draw but may have wrong orientation until mapped.",
                            layer.name);
                    }
                }
                else
                {
                    // Engine-native: 0-based atlas index, -1 empty
                    if (raw < 0) continue;
                    tileIndex = raw;
                }

                if (tileIndex < 0) continue;

                SDL_Rect src = CalcTileSrcRect(tilemap.tileset, tileIndex);

                // Bounds check: if src rect is outside the texture, SDL will skip draw (often with no log)
                if (!warnedOutOfBounds &&
                    (src.x < 0 || src.y < 0 || (src.x + src.w) > tex->Width() || (src.y + src.h) > tex->Height()))
                {
                    warnedOutOfBounds = true;
                    spdlog::error(
                        "Tile layer '{}' computed out-of-bounds srcRect for tileIndex {} (src {} {} {} {}) on texture '{}' ({}x{}). Check columns/spacing/margin and whether ids are 0-based vs gid.",
                        layer.name, tileIndex, src.x, src.y, src.w, src.h, tex->Path(), tex->Width(), tex->Height());
                }

                const glm::vec2 worldPos =
                    origin + glm::vec2((float)(x * tilemap.tileWidth), (float)(y * tilemap.tileHeight));

                renderer.DrawTexture(
                    *tex,
                    worldPos,
                    { (float)tilemap.tileWidth, (float)tilemap.tileHeight },
                    &src,
                    rotationDeg,
                    flip,
                    layer.tint
                );
            }
        }
    }
}