#include "pch.h"
#include "Graphics/Renderer2D.h"

#include <SDL2/SDL.h>
#include <spdlog/spdlog.h>

namespace eng
{
    struct Renderer2D::Impl
    {
        SDL_Renderer* renderer = nullptr;
    };

    Renderer2D::Renderer2D() : m(std::make_unique<Impl>()) {}
    Renderer2D::~Renderer2D()
    {
        if (m && m->renderer)
        {
            SDL_DestroyRenderer(m->renderer);
            m->renderer = nullptr;
        }
    }

    Renderer2D::Renderer2D(Renderer2D&&) noexcept = default;
    Renderer2D& Renderer2D::operator=(Renderer2D&&) noexcept = default;

    bool Renderer2D::Init(void* nativeWindowHandle, bool vsync)
    {
        SDL_Window* window = static_cast<SDL_Window*>(nativeWindowHandle);

        const Uint32 flags = SDL_RENDERER_ACCELERATED | (vsync ? SDL_RENDERER_PRESENTVSYNC : 0);
        m->renderer = SDL_CreateRenderer(window, -1, flags);
        if (!m->renderer)
        {
            spdlog::critical("SDL_CreateRenderer failed: {}", SDL_GetError());
            return false;
        }
        return true;
    }

    void Renderer2D::Clear(Color c)
    {
        SDL_SetRenderDrawColor(m->renderer, c.r, c.g, c.b, c.a);
        SDL_RenderClear(m->renderer);
    }

    void Renderer2D::DrawRectFilled(RectI r, Color c)
    {
        SDL_Rect sr{ r.x, r.y, r.w, r.h };
        SDL_SetRenderDrawColor(m->renderer, c.r, c.g, c.b, c.a);
        SDL_RenderFillRect(m->renderer, &sr);
    }

    void Renderer2D::Present()
    {
        SDL_RenderPresent(m->renderer);
    }

    void Renderer2D::DrawLine(int x1, int y1, int x2, int y2, Color c)
    {
        SDL_SetRenderDrawColor(m->renderer, c.r, c.g, c.b, c.a);
        SDL_RenderDrawLine(m->renderer, x1, y1, x2, y2);
    }

    void Renderer2D::DrawPolyLine(const eng::Vec2f* pts, int count, Color c, bool closed)
    {
        if (!pts || count < 2) return;

        SDL_SetRenderDrawColor(m->renderer, c.r, c.g, c.b, c.a);

        for (int i = 0; i < count - 1; ++i)
        {
            SDL_RenderDrawLine(
                m->renderer,
                (int)pts[i].x, (int)pts[i].y,
                (int)pts[i + 1].x, (int)pts[i + 1].y
            );
        }

        if (closed)
        {
            SDL_RenderDrawLine(
                m->renderer,
                (int)pts[count - 1].x, (int)pts[count - 1].y,
                (int)pts[0].x, (int)pts[0].y
            );
        }
    }
}