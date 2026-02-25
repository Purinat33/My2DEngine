#pragma once
#include <vector>
#include <array>
#include "Core/Types.h"
#include "Core/Math2D.h"

namespace eng::world
{
    static constexpr int TileSizePx = 64;

    enum class ColliderType : uint8_t
    {
        Empty,
        Solid,

        // 45-degree
        Slope45_Up,    // /  (walks up as X increases)
        Slope45_Down,  // \

        // “Half” slope (~26.565°): rises 32px over 64px
        SlopeHalf_Up,
        SlopeHalf_Down,

        // Steep (~63.435°): rises 64px over 32px
        SlopeSteep_Up,
        SlopeSteep_Down
    };

    struct Poly
    {
        std::array<eng::Vec2f, 6> v{};
        int count = 0;
    };

    class Tilemap
    {
    public:
        Tilemap() = default;
        Tilemap(int w, int h) { Resize(w, h); }

        void Resize(int w, int h)
        {
            width = w; height = h;
            collision.assign((size_t)w * (size_t)h, ColliderType::Empty);
        }

        bool InBounds(int x, int y) const
        {
            return x >= 0 && y >= 0 && x < width && y < height;
        }

        ColliderType Get(int x, int y) const
        {
            if (!InBounds(x, y)) return ColliderType::Empty;
            return collision[(size_t)y * (size_t)width + (size_t)x];
        }

        void Set(int x, int y, ColliderType t)
        {
            if (!InBounds(x, y)) return;
            collision[(size_t)y * (size_t)width + (size_t)x] = t;
        }

        // World (px) -> tile coords
        eng::Vec2f TileOriginPx(int tx, int ty) const
        {
            return eng::Vec2f{ (float)(tx * TileSizePx), (float)(ty * TileSizePx) };
        }

        Poly GetColliderPolyWorldPx(int tx, int ty) const
        {
            const ColliderType t = Get(tx, ty);
            Poly p = MakeLocalPoly(t);

            const eng::Vec2f o = TileOriginPx(tx, ty);
            for (int i = 0; i < p.count; ++i)
                p.v[i] = p.v[i] + o;

            return p;
        }

        int width = 0;
        int height = 0;

    private:
        static Poly MakeLocalPoly(ColliderType t)
        {
            const float T = (float)TileSizePx;
            const float H = T * 0.5f; // 32px

            Poly p{};

            switch (t)
            {
            case ColliderType::Solid:
                p.count = 4;
                p.v[0] = { 0, 0 };
                p.v[1] = { T, 0 };
                p.v[2] = { T, T };
                p.v[3] = { 0, T };
                return p;

            case ColliderType::Slope45_Up: // /  solid under the line from (0,T)->(T,0)
                p.count = 3;
                p.v[0] = { 0, T };
                p.v[1] = { T, 0 };
                p.v[2] = { T, T };
                return p;

            case ColliderType::Slope45_Down: // \ solid under the line from (0,0)->(T,T)
                p.count = 3;
                p.v[0] = { 0, 0 };
                p.v[1] = { 0, T };
                p.v[2] = { T, T };
                return p;

            case ColliderType::SlopeHalf_Up: // rises 32px across tile
                p.count = 3;
                p.v[0] = { 0, T };
                p.v[1] = { T, H };
                p.v[2] = { T, T };
                return p;

            case ColliderType::SlopeHalf_Down:
                p.count = 3;
                p.v[0] = { 0, H };
                p.v[1] = { 0, T };
                p.v[2] = { T, T };
                return p;

            case ColliderType::SlopeSteep_Up: // rises 64px over 32px: line from (0,T)->(H,0)
                p.count = 3;
                p.v[0] = { 0, T };
                p.v[1] = { H, 0 };
                p.v[2] = { T, T };
                return p;

            case ColliderType::SlopeSteep_Down: // line from (H,0)->(T,T)
                p.count = 3;
                p.v[0] = { 0, T };
                p.v[1] = { H, 0 };
                p.v[2] = { 0, 0 };
                // This one is easy to get “wrong” per tileset — you can tweak later.
                return p;

            default:
                p.count = 0;
                return p;
            }
        }

        std::vector<ColliderType> collision;
    };
}