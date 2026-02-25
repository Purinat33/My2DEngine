#pragma once
#include <limits>
#include <vector>
#include "Core/Types.h"
#include "Core/Math2D.h"
#include "World/Tilemap.h"

namespace eng::collision
{
    struct Capsule
    {
        eng::Vec2f center;      // center of capsule
        float radius = 18.0f;   // px
        float halfHeight = 44.0f; // px (total height = 2*halfHeight). Must be >= radius.

        void SegmentEndpoints(eng::Vec2f& a, eng::Vec2f& b) const
        {
            const float segHalf = std::max(0.0f, halfHeight - radius);
            // vertical capsule, Y+ down
            a = { center.x, center.y - segHalf };
            b = { center.x, center.y + segHalf };
        }
    };

    struct Contact
    {
        eng::Vec2f normal{ 0,0 }; // points from polygon -> capsule
        float penetration = 0.0f;
    };

    inline eng::Vec2f ClosestPointOnSegment(eng::Vec2f p, eng::Vec2f a, eng::Vec2f b)
    {
        eng::Vec2f ab = b - a;
        float denom = eng::Dot(ab, ab);
        if (denom < 1e-6f) return a;
        float t = eng::Dot(p - a, ab) / denom;
        t = eng::Clamp(t, 0.0f, 1.0f);
        return a + ab * t;
    }

    inline eng::Vec2f ClosestPointOnPolyEdges(eng::Vec2f p, const eng::Vec2f* v, int n)
    {
        float bestD2 = std::numeric_limits<float>::infinity();
        eng::Vec2f best = v[0];

        for (int i = 0; i < n; ++i)
        {
            eng::Vec2f a = v[i];
            eng::Vec2f b = v[(i + 1) % n];
            eng::Vec2f cp = ClosestPointOnSegment(p, a, b);
            float d2 = eng::LenSq(cp - p);
            if (d2 < bestD2)
            {
                bestD2 = d2;
                best = cp;
            }
        }
        return best;
    }

    inline void ProjectPoly(const eng::Vec2f* v, int n, eng::Vec2f axis, float& outMin, float& outMax)
    {
        float mn = eng::Dot(v[0], axis);
        float mx = mn;
        for (int i = 1; i < n; ++i)
        {
            float d = eng::Dot(v[i], axis);
            mn = std::min(mn, d);
            mx = std::max(mx, d);
        }
        outMin = mn; outMax = mx;
    }

    inline void ProjectCapsule(const Capsule& c, eng::Vec2f axis, float& outMin, float& outMax)
    {
        eng::Vec2f a, b;
        c.SegmentEndpoints(a, b);

        float da = eng::Dot(a, axis);
        float db = eng::Dot(b, axis);

        float mn = std::min(da, db) - c.radius;
        float mx = std::max(da, db) + c.radius;

        outMin = mn; outMax = mx;
    }

    inline eng::Vec2f PolyCentroid(const eng::Vec2f* v, int n)
    {
        eng::Vec2f c{ 0,0 };
        for (int i = 0; i < n; ++i) c = c + v[i];
        return c * (1.0f / (float)n);
    }

    inline bool IntersectCapsulePolySAT(const Capsule& cap, const eng::Vec2f* poly, int n, Contact& out)
    {
        if (n < 3) return false;

        std::vector<eng::Vec2f> axes;
        axes.reserve((size_t)n + 4);

        // 1) Polygon edge normals
        for (int i = 0; i < n; ++i)
        {
            eng::Vec2f a = poly[i];
            eng::Vec2f b = poly[(i + 1) % n];
            eng::Vec2f e = b - a;
            eng::Vec2f axis = eng::Normalize(eng::Perp(e));
            if (eng::LenSq(axis) > 1e-6f) axes.push_back(axis);
        }

        // 2) Capsule side axis (vertical capsule => side normals are horizontal)
        axes.push_back(eng::Vec2f{ 1.0f, 0.0f });

        // 3) “Circle” axes from end-cap centers to closest point on polygon
        eng::Vec2f s0, s1;
        cap.SegmentEndpoints(s0, s1);

        for (eng::Vec2f c : { s0, s1 })
        {
            eng::Vec2f cp = ClosestPointOnPolyEdges(c, poly, n);
            eng::Vec2f axis = eng::Normalize(c - cp);
            if (eng::LenSq(axis) > 1e-6f) axes.push_back(axis);
        }

        float bestOverlap = std::numeric_limits<float>::infinity();
        eng::Vec2f bestAxis{ 0,0 };

        for (const eng::Vec2f axisRaw : axes)
        {
            eng::Vec2f axis = eng::Normalize(axisRaw);
            if (eng::LenSq(axis) < 1e-6f) continue;

            float pMin, pMax, cMin, cMax;
            ProjectPoly(poly, n, axis, pMin, pMax);
            ProjectCapsule(cap, axis, cMin, cMax);

            float overlap = std::min(pMax, cMax) - std::max(pMin, cMin);
            if (overlap <= 0.0f)
                return false;

            if (overlap < bestOverlap)
            {
                bestOverlap = overlap;
                bestAxis = axis;
            }
        }

        // Orient normal to push capsule OUT of polygon
        eng::Vec2f centroid = PolyCentroid(poly, n);
        eng::Vec2f d = cap.center - centroid;
        if (eng::Dot(d, bestAxis) < 0.0f)
            bestAxis = bestAxis * -1.0f;

        out.normal = bestAxis;
        out.penetration = bestOverlap;
        return true;
    }

    // Find best contact between capsule and tilemap (broadphase = tiles overlapping capsule AABB)
    inline bool FindBestTileContact(const Capsule& cap, const eng::world::Tilemap& map, Contact& outBest)
    {
        const float minX = cap.center.x - cap.radius;
        const float maxX = cap.center.x + cap.radius;
        const float minY = cap.center.y - cap.halfHeight;
        const float maxY = cap.center.y + cap.halfHeight;

        const int tMinX = (int)std::floor(minX / (float)eng::world::TileSizePx);
        const int tMaxX = (int)std::floor(maxX / (float)eng::world::TileSizePx);
        const int tMinY = (int)std::floor(minY / (float)eng::world::TileSizePx);
        const int tMaxY = (int)std::floor(maxY / (float)eng::world::TileSizePx);

        bool found = false;
        float bestPen = 0.0f;
        Contact best{};

        for (int ty = tMinY; ty <= tMaxY; ++ty)
            for (int tx = tMinX; tx <= tMaxX; ++tx)
            {
                const auto t = map.Get(tx, ty);
                if (t == eng::world::ColliderType::Empty) continue;

                auto poly = map.GetColliderPolyWorldPx(tx, ty);
                if (poly.count < 3) continue;

                Contact c{};
                if (IntersectCapsulePolySAT(cap, poly.v.data(), poly.count, c))
                {
                    if (!found || c.penetration > bestPen)
                    {
                        found = true;
                        bestPen = c.penetration;
                        best = c;
                    }
                }
            }

        if (found) outBest = best;
        return found;
    }
}