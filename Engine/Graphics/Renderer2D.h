#pragma once
#include <memory>
#include "Core/Types.h"

namespace eng
{
    class Renderer2D
    {
    public:
        Renderer2D();
        ~Renderer2D();

        Renderer2D(Renderer2D&&) noexcept;
        Renderer2D& operator=(Renderer2D&&) noexcept;

        Renderer2D(const Renderer2D&) = delete;
        Renderer2D& operator=(const Renderer2D&) = delete;

        // Internal init from the App (kept public for now, but only Engine should call it)
        bool Init(void* nativeWindowHandle, bool vsync);

        void Clear(Color c);
        void DrawRectFilled(RectI r, Color c);
        void DrawLine(int x1, int y1, int x2, int y2, Color c);
        void DrawPolyLine(const eng::Vec2f* pts, int count, Color c, bool closed = true);
        void Present();

    private:
        struct Impl;
        std::unique_ptr<Impl> m;
    };
}