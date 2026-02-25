#pragma once
#include "Core/Types.h"
#include <cmath>
#include <algorithm>

namespace eng
{
    inline Vec2f operator+(Vec2f a, Vec2f b) { return { a.x + b.x, a.y + b.y }; }
    inline Vec2f operator-(Vec2f a, Vec2f b) { return { a.x - b.x, a.y - b.y }; }
    inline Vec2f operator*(Vec2f v, float s) { return { v.x * s, v.y * s }; }
    inline Vec2f operator*(float s, Vec2f v) { return v * s; }

    inline float Dot(Vec2f a, Vec2f b) { return a.x * b.x + a.y * b.y; }
    inline float LenSq(Vec2f v) { return Dot(v, v); }
    inline float Len(Vec2f v) { return std::sqrt(LenSq(v)); }

    inline Vec2f Normalize(Vec2f v)
    {
        float l = Len(v);
        if (l < 1e-6f) return { 0.0f, 0.0f };
        return { v.x / l, v.y / l };
    }

    // Left-perp
    inline Vec2f Perp(Vec2f v) { return { -v.y, v.x }; }

    inline float Clamp(float x, float a, float b) { return std::max(a, std::min(x, b)); }

    inline float MoveTowards(float current, float target, float maxDelta)
    {
        float delta = target - current;
        if (std::fabs(delta) <= maxDelta) return target;
        return current + (delta > 0.0f ? maxDelta : -maxDelta);
    }
}