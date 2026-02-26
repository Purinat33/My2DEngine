#pragma once

#include "Platform/Sdl.h"
#include "Assets/AssetManager.h"
#include "Renderer/Renderer2D.h"

#include "Core/EngineConfig.h"
#include "Core/Input.h"
#include "Core/Time.h"

#include "Platform/Window.h"

#include "Physics/PhysicsWorld.h"
#include "Physics/PhysicsDebugDraw.h"

#include "Gameplay/WorldState.h"

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
        AssetManager& GetAssets() { return m_assets; }
        Renderer2D& GetRenderer2D() { return m_renderer2d; }
        PhysicsWorld& GetPhysics() { return m_physics; }
        PhysicsDebugDraw& GetPhysicsDebugDraw() { return m_physicsDebug; }
        float PixelsPerMeter() const { return m_pixelsPerMeter; }
        bool DrawPhysicsDebug() const { return m_drawPhysicsDebug; }
        const Time& GetTime() const;

        WorldState& GetWorldState() { return m_worldState; }
        const WorldState& GetWorldState() const { return m_worldState; }

    private:
        // Concrete members
        Platform::Window m_window; // (defined in Platform/Window.h)
        Input m_input;
        Time m_time;
        AssetManager m_assets;
        Renderer2D m_renderer2d;
        PhysicsWorld m_physics;
        PhysicsDebugDraw m_physicsDebug;
        float m_pixelsPerMeter = 100.0f;
        bool m_drawPhysicsDebug = true;

        WorldState m_worldState;

        double m_fixedAccumulator = 0.0;
        bool m_initialized = false;
        bool m_quitRequested = false;
    };
}