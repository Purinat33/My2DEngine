#pragma once

// Note: Engine headers define SDL_MAIN_HANDLED, so apps can use normal int main().
#include "Platform/Sdl.h"

namespace my2d
{
    class Engine;

    class App
    {
    public:
        virtual ~App() = default;

        virtual bool OnInit(Engine& engine) { (void)engine; return true; }
        virtual void OnShutdown(Engine& engine) { (void)engine; }

        virtual void OnUpdate(Engine& engine, double dt) { (void)engine; (void)dt; }
        virtual void OnFixedUpdate(Engine& engine, double fixedDt) { (void)engine; (void)fixedDt; }
        virtual void OnPostFixedUpdate(Engine& engine, double fixedDt) { (void)engine; (void)fixedDt; }
        virtual void OnRender(Engine& engine) { (void)engine; }

        // Return true if consumed (engine won’t do default handling for input/window).
        virtual bool OnEvent(Engine& engine, const SDL_Event& e) { (void)engine; (void)e; return false; }
    };
}