#pragma once
#include "Platform/Sdl.h"

#include <string>
#include <spdlog/spdlog.h>

#include "Core/EngineConfig.h"

namespace my2d::Platform
{
    class Window
    {
    public:
        bool Create(const EngineConfig& config)
        {
            uint32_t flags = SDL_WINDOW_SHOWN;
            if (config.resizable) flags |= SDL_WINDOW_RESIZABLE;

            m_window = SDL_CreateWindow(
                config.windowTitle.c_str(),
                SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                config.windowWidth, config.windowHeight,
                flags
            );

            if (!m_window)
            {
                spdlog::error("SDL_CreateWindow failed: {}", SDL_GetError());
                return false;
            }

            m_width = config.windowWidth;
            m_height = config.windowHeight;

            uint32_t rendererFlags = SDL_RENDERER_ACCELERATED;
            if (config.vsync) rendererFlags |= SDL_RENDERER_PRESENTVSYNC;

            m_renderer = SDL_CreateRenderer(m_window, -1, rendererFlags);
            if (!m_renderer)
            {
                spdlog::error("SDL_CreateRenderer failed: {}", SDL_GetError());
                return false;
            }

            SDL_SetRenderDrawBlendMode(m_renderer, SDL_BLENDMODE_BLEND);
            return true;
        }

        void Destroy()
        {
            if (m_renderer) { SDL_DestroyRenderer(m_renderer); m_renderer = nullptr; }
            if (m_window) { SDL_DestroyWindow(m_window); m_window = nullptr; }
        }

        void OnEvent(const SDL_Event& e)
        {
            if (e.type == SDL_WINDOWEVENT)
            {
                if (e.window.event == SDL_WINDOWEVENT_SIZE_CHANGED)
                {
                    m_width = e.window.data1;
                    m_height = e.window.data2;
                }
            }
        }

        void BeginFrame()
        {
            SDL_SetRenderDrawColor(m_renderer, 0, 0, 0, 255);
            SDL_RenderClear(m_renderer);
        }

        void EndFrame()
        {
            SDL_RenderPresent(m_renderer);
        }

        SDL_Window* GetSDLWindow() const { return m_window; }
        SDL_Renderer* GetSDLRenderer() const { return m_renderer; }

        int Width() const { return m_width; }
        int Height() const { return m_height; }

    private:
        SDL_Window* m_window = nullptr;
        SDL_Renderer* m_renderer = nullptr;
        int m_width = 0;
        int m_height = 0;
    };
}