#include "pch.h"
#include "Core/App.h"

#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>

#include <spdlog/spdlog.h>
#include <chrono>

namespace eng
{
    int Run(const AppConfig& cfg, IGame& game)
    {
        spdlog::set_pattern("[%T] [%^%l%$] %v");

        if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) != 0)
        {
            spdlog::critical("SDL_Init failed: {}", SDL_GetError());
            return -1;
        }

        Uint32 windowFlags = SDL_WINDOW_SHOWN;
        if (cfg.resizable) windowFlags |= SDL_WINDOW_RESIZABLE;

        SDL_Window* window = SDL_CreateWindow(
            cfg.title.c_str(),
            SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
            cfg.width, cfg.height,
            windowFlags
        );

        if (!window)
        {
            spdlog::critical("SDL_CreateWindow failed: {}", SDL_GetError());
            SDL_Quit();
            return -1;
        }

        Renderer2D renderer;
        if (!renderer.Init(window, cfg.vsync))
        {
            SDL_DestroyWindow(window);
            SDL_Quit();
            return -1;
        }

        game.OnInit();

        bool running = true;

        // Fixed timestep (60 Hz)
        const double dt = 1.0 / 60.0;
        double accumulator = 0.0;

        auto prev = std::chrono::steady_clock::now();

        while (running)
        {
            // Events
            SDL_Event e;
            while (SDL_PollEvent(&e))
            {
                if (e.type == SDL_QUIT)
                    running = false;

                if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE)
                    running = false;
            }

            // Timing
            auto now = std::chrono::steady_clock::now();
            std::chrono::duration<double> elapsed = now - prev;
            prev = now;

            double frameTime = elapsed.count();
            if (frameTime > 0.25) frameTime = 0.25; // avoid spiral of death on breakpoints

            accumulator += frameTime;

            while (accumulator >= dt)
            {
                game.FixedUpdate(static_cast<float>(dt));
                accumulator -= dt;
            }

            float alpha = static_cast<float>(accumulator / dt);

            renderer.Clear(Color{ 20, 20, 24, 255 });
            game.Render(renderer, alpha);
            renderer.Present();
        }

        game.OnShutdown();

        SDL_DestroyWindow(window);
        SDL_Quit();

        return 0;
    }
}