#include "pch.h"
#include "Physics/TilemapColliderBuilder.h"
#include "Scene/Components.h"

#include <vector>
#include <algorithm>

namespace my2d
{
    struct RectI { int x, y, w, h; };

    static bool IsSolid(const TileLayer& layer, int idx)
    {
        // -1 = empty, >=0 = tile exists => solid
        return layer.tiles[idx] >= 0;
    }

    static std::vector<RectI> MergeSolidsGreedy(const TilemapComponent& tm, const TileLayer& layer)
    {
        const int W = tm.width;
        const int H = tm.height;

        std::vector<uint8_t> used((size_t)W * (size_t)H, 0);
        std::vector<RectI> rects;

        auto inBounds = [&](int x, int y) { return x >= 0 && y >= 0 && x < W && y < H; };
        auto idxOf = [&](int x, int y) { return y * W + x; };

        for (int y = 0; y < H; ++y)
        {
            for (int x = 0; x < W; ++x)
            {
                const int idx = idxOf(x, y);
                if (used[idx]) continue;
                if (!IsSolid(layer, idx)) continue;

                // Find max width
                int w = 1;
                while (x + w < W)
                {
                    const int ii = idxOf(x + w, y);
                    if (used[ii] || !IsSolid(layer, ii)) break;
                    ++w;
                }

                // Extend down while full row solid
                int h = 1;
                for (;;)
                {
                    const int ny = y + h;
                    if (ny >= H) break;

                    bool rowOk = true;
                    for (int xx = 0; xx < w; ++xx)
                    {
                        const int ii = idxOf(x + xx, ny);
                        if (used[ii] || !IsSolid(layer, ii)) { rowOk = false; break; }
                    }

                    if (!rowOk) break;
                    ++h;
                }

                // Mark used
                for (int yy = 0; yy < h; ++yy)
                    for (int xx = 0; xx < w; ++xx)
                        used[idxOf(x + xx, y + yy)] = 1;

                rects.push_back(RectI{ x, y, w, h });
            }
        }

        return rects;
    }

    void BuildTilemapColliders(PhysicsWorld& physics, Scene& scene, float ppm)
    {
        if (!physics.IsValid()) return;

        auto& reg = scene.Registry();
        auto view = reg.view<TransformComponent, TilemapComponent, TilemapColliderComponent>();

        for (auto e : view)
        {
            auto& tc = view.get<TransformComponent>(e);
            auto& tm = view.get<TilemapComponent>(e);
            auto& col = view.get<TilemapColliderComponent>(e);

            // Cleanup old bodies
            for (b2BodyId id : col.runtimeBodies)
            {
                if (b2Body_IsValid(id))
                    b2DestroyBody(id);
            }
            col.runtimeBodies.clear();

            if (tm.width <= 0 || tm.height <= 0) continue;
            if (col.collisionLayerIndex < 0 || col.collisionLayerIndex >= (int)tm.layers.size()) continue;

            TileLayer& layer = tm.layers[col.collisionLayerIndex];
            if ((int)layer.tiles.size() != tm.width * tm.height) continue;

            const auto rects = MergeSolidsGreedy(tm, layer);

            for (const auto& r : rects)
            {
                // rect center in PIXELS
                const float px = tc.position.x + (r.x + r.w * 0.5f) * (float)tm.tileWidth;
                const float py = tc.position.y + (r.y + r.h * 0.5f) * (float)tm.tileHeight;

                // half extents in METERS
                const float halfW = (r.w * tm.tileWidth * 0.5f) / ppm;
                const float halfH = (r.h * tm.tileHeight * 0.5f) / ppm;

                // Create one static body per merged rect (simple + no polygon offset math)
                b2BodyDef bd = b2DefaultBodyDef();
                bd.type = b2_staticBody;
                bd.position = b2Vec2{ px / ppm, py / ppm };

                b2BodyId bodyId = b2CreateBody(physics.WorldId(), &bd);

                b2ShapeDef sd = b2DefaultShapeDef();
                sd.density = 0.0f;
                sd.material.friction = col.friction;
                sd.material.restitution = col.restitution;
                sd.isSensor = col.isSensor;

                b2Polygon box = b2MakeBox(halfW, halfH);
                b2CreatePolygonShape(bodyId, &sd, &box);

                col.runtimeBodies.push_back(bodyId);
            }
        }
    }
}