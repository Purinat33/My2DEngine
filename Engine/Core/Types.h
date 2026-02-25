#pragma once
#include <cstdint>

namespace eng
{
    struct Color
    {
        uint8_t r = 0, g = 0, b = 0, a = 255;
    };

    struct RectI
    {
        int x = 0, y = 0, w = 0, h = 0;
    };

    struct RectF
    {
        float x = 0.0f, y = 0.0f, w = 0.0f, h = 0.0f;
    };

    struct Vec2f
    {
        float x = 0.0f;
        float y = 0.0f;
    };
}