#include "pch.h"
#include "Physics/TilemapColliderBuilder.h"
#include "Scene/Components.h"
#include "Physics/PhysicsLayers.h"

#include <vector>
#include <algorithm>

namespace my2d
{
    struct RectI { int x, y, w, h; };

    static bool Contains(const std::vector<int>& v, int x)
    {
        return std::find(v.begin(), v.end(), x) != v.end();
    }

    static bool IsSlopeTile(const TilemapColliderComponent& col, int tileIndex)
    {
        return Contains(col.slopeUpRightTiles, tileIndex) || Contains(col.slopeUpLeftTiles, tileIndex);
    }

    static bool IsSolid(const TileLayer& layer, const TilemapColliderComponent& col, int idx)
    {
        const int tileIndex = layer.tiles[idx];
        if (tileIndex < 0) return false;
        if (IsSlopeTile(col, tileIndex)) return false; // slopes handled separately
        return true;
    }

    static std::vector<RectI> MergeSolidsGreedy(const TilemapComponent& tm, const TileLayer& layer, const TilemapColliderComponent& col)
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
                if (!IsSolid(layer, col, idx)) continue;

                // Find max width
                int w = 1;
                while (x + w < W)
                {
                    const int ii = idxOf(x + w, y);
                    if (used[ii] || !IsSolid(layer, col, ii)) break;
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
                        if (used[ii] || !IsSolid(layer, col, ii)) { rowOk = false; break; }
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

            // --- SLOPE TRIANGLES (one body per slope tile, simple & works well) ---
            const float tileWm = (float)tm.tileWidth / ppm;
            const float tileHm = (float)tm.tileHeight / ppm;

            for (int y = 0; y < tm.height; ++y)
            {
                for (int x = 0; x < tm.width; ++x)
                {
                    const int idx = y * tm.width + x;
                    const int tileIndex = layer.tiles[idx];
                    if (tileIndex < 0) continue;
                    if (!IsSlopeTile(col, tileIndex)) continue;

                    // tile center in pixels
                    const float centerPxX = tc.position.x + (x + 0.5f) * tm.tileWidth;
                    const float centerPxY = tc.position.y + (y + 0.5f) * tm.tileHeight;

                    b2BodyDef bd = b2DefaultBodyDef();
                    bd.type = b2_staticBody;
                    bd.position = b2Vec2{ centerPxX / ppm, centerPxY / ppm };
                    b2BodyId bodyId = b2CreateBody(physics.WorldId(), &bd);

                    b2ShapeDef sd = b2DefaultShapeDef();
                    sd.density = 0.0f;
                    sd.material.friction = col.slopeFriction;
                    sd.material.restitution = col.restitution;
                    sd.isSensor = col.isSensor;

                    sd.filter.categoryBits = my2d::PhysicsLayers::Environment;
                    sd.filter.maskBits = my2d::PhysicsLayers::All;

                    // triangle vertices in local body coords (meters), centered at tile center
                    b2Vec2 pts[3];

                    if (Contains(col.slopeUpRightTiles, tileIndex))
                    {
                        // "/" : solid below diagonal from bottom-left to top-right
                        pts[0] = b2Vec2{ -tileWm * 0.5f,  tileHm * 0.5f }; // bottom-left
                        pts[1] = b2Vec2{ tileWm * 0.5f,  tileHm * 0.5f }; // bottom-right
                        pts[2] = b2Vec2{ tileWm * 0.5f, -tileHm * 0.5f }; // top-right
                    }
                    else
                    {
                        // "\" : solid below diagonal from top-left to bottom-right
                        pts[0] = b2Vec2{ -tileWm * 0.5f, -tileHm * 0.5f }; // top-left
                        pts[1] = b2Vec2{ -tileWm * 0.5f,  tileHm * 0.5f }; // bottom-left
                        pts[2] = b2Vec2{ tileWm * 0.5f,  tileHm * 0.5f }; // bottom-right
                    }

                    b2Hull hull = b2ComputeHull(pts, 3);
                    b2Polygon tri = b2MakePolygon(&hull, 0.0f);
                    b2CreatePolygonShape(bodyId, &sd, &tri);

                    col.runtimeBodies.push_back(bodyId);
                }
            }

            const auto rects = MergeSolidsGreedy(tm, layer, col);

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
                sd.filter.categoryBits = my2d::PhysicsLayers::Environment;
                sd.filter.maskBits = my2d::PhysicsLayers::All;
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