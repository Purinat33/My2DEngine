#pragma once

#include "Platform/Sdl.h"

#include "Core/EngineConfig.h"
#include "Core/Input.h"
#include "Core/Time.h"

#include "Platform/Window.h"

namespace my2d
{
    class App;

    class Engine
    {
    public:
        Engine();
        ~Engine();

        bool Initialize(const EngineConfig& config);
        void Shutdown();

        int Run(const EngineConfig& config, App& app);

        void RequestQuit();

        SDL_Window* GetSDLWindow() const;
        SDL_Renderer* GetSDLRenderer() const;

        Input& GetInput();
        const Time& GetTime() const;

    private:
        // Concrete members
        Platform::Window m_window; // (defined in Platform/Window.h)
        Input m_input;
        Time m_time;

        double m_fixedAccumulator = 0.0;
        bool m_initialized = false;
        bool m_quitRequested = false;
    };
}