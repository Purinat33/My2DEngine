#include "pch.h"
#include "framework.h"

#include "Platform/Window.h"

#include <algorithm>
#include <spdlog/spdlog.h>
#include "Platform/SdlImage.h"
#include "Platform/SdlTtf.h"

namespace my2d
{
    static void InitLoggingOnce()
    {
        static bool s_inited = false;
        if (s_inited) return;
        s_inited = true;

        spdlog::set_pattern("[%H:%M:%S.%e] [%^%l%$] %v");
        spdlog::set_level(spdlog::level::info);
    }

    Engine::Engine() = default;

    Engine::~Engine()
    {
        // Safety in case user forgot.
        if (m_initialized)
            Shutdown();
    }

    bool Engine::Initialize(const EngineConfig& config)
    {
        InitLoggingOnce();

        if (m_initialized)
            return true;

        SDL_SetMainReady();

        if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS | SDL_INIT_GAMECONTROLLER) != 0)
        {
            spdlog::error("SDL_Init failed: {}", SDL_GetError());
            return false;
        }

        const int imgFlags = IMG_INIT_PNG | IMG_INIT_JPG;
        const int requested = IMG_INIT_PNG | IMG_INIT_JPG;
        const int initted = IMG_Init(requested);

        // Require PNG (most of your pipeline will use it)
        if ((initted & IMG_INIT_PNG) != IMG_INIT_PNG)
        {
            spdlog::error("IMG_Init failed (PNG not available): {}", IMG_GetError());
            return false;
        }

        // JPEG optional (warn only)
        if ((initted & IMG_INIT_JPG) != IMG_INIT_JPG)
        {
            spdlog::warn("JPEG support not available: {}", IMG_GetError());
        }

        if (TTF_Init() != 0)
        {
            spdlog::error("TTF_Init failed: {}", TTF_GetError());
            return false;
        }

        if (!m_window.Create(config))
            return false;

        // Hook runtime systems to the created SDL_Renderer
        m_assets.SetRenderer(m_window.GetSDLRenderer());
        m_assets.SetContentRoot(config.contentRoot);

        m_renderer2d.SetRenderer(m_window.GetSDLRenderer());
        m_renderer2d.SetViewport(m_window.Width(), m_window.Height());

        m_pixelsPerMeter = config.pixelsPerMeter;
        m_drawPhysicsDebug = config.drawPhysicsDebug;

        m_gravity = b2Vec2(config.gravityX, config.gravityY);
        m_physics.Initialize(m_gravity);

        m_time.Reset(SDL_GetPerformanceCounter());
        m_fixedAccumulator = 0.0;
        m_quitRequested = false;
        m_initialized = true;

        spdlog::info("Engine initialized.");
        return true;
    }

    void Engine::Shutdown()
    {
        if (!m_initialized)
            return;

        spdlog::info("Engine shutdown...");

        m_window.Destroy();

        TTF_Quit();
        IMG_Quit();
        m_physics.Shutdown();
        SDL_Quit();

        m_initialized = false;
    }

    int Engine::Run(const EngineConfig& config, App& app)
    {
        if (!Initialize(config))
            return 1;

        if (!app.OnInit(*this))
        {
            spdlog::error("App OnInit failed.");
            Shutdown();
            return 2;
        }

        // Main loop
        while (!m_quitRequested)
        {
            // Tick time
            m_time.Tick(SDL_GetPerformanceCounter(), SDL_GetPerformanceFrequency());

            // Clamp dt to avoid spiral-of-death on pauses/breakpoints
            const double dt = std::min(m_time.DeltaSeconds(), 0.25);

            // Input frame boundary
            m_input.BeginFrame();

            // Pump events
            SDL_Event e{};
            while (SDL_PollEvent(&e))
            {
                if (e.type == SDL_QUIT)
                {
                    RequestQuit();
                    continue;
                }

                // Allow app to consume first if desired
                const bool consumed = app.OnEvent(*this, e);
                if (!consumed)
                {
                    m_input.OnEvent(e);
                    m_window.OnEvent(e);
                }
            }

            // Update viewport in case the window resized
            m_renderer2d.SetViewport(m_window.Width(), m_window.Height());

            // Fixed update (physics)
            m_fixedAccumulator += dt;
            const double fixedDt = config.fixedDeltaSeconds;

            while (m_fixedAccumulator >= fixedDt)
            {
                app.OnFixedUpdate(*this, fixedDt);
                m_physics.Step((float)fixedDt);
                m_fixedAccumulator -= fixedDt;
            }

            // Variable update + render
            app.OnUpdate(*this, dt);

            m_window.BeginFrame();
            app.OnRender(*this);
            m_window.EndFrame();
        }

        app.OnShutdown(*this);
        Shutdown();
        return 0;
    }

    void Engine::RequestQuit()
    {
        m_quitRequested = true;
    }

    void Engine::ResetPhysicsWorld()
    {
        m_physics.Shutdown();
        m_physics.Initialize(m_gravity);
    }

    // Accessors
    SDL_Window* Engine::GetSDLWindow() const { return m_window.GetSDLWindow(); }
    SDL_Renderer* Engine::GetSDLRenderer() const { return m_window.GetSDLRenderer(); }
    Input& Engine::GetInput() { return m_input; }
    const Time& Engine::GetTime() const { return m_time; }
}