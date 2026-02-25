#pragma once
#include <string>
#include "Graphics/Renderer2D.h"

namespace eng
{
    struct AppConfig
    {
        std::string title = "My2DEngine";
        int width = 1280;
        int height = 720;
        bool resizable = true;
        bool vsync = true;
    };

    class IGame
    {
    public:
        virtual ~IGame() = default;

        virtual void OnInit() {}
        virtual void FixedUpdate(float dt) { (void)dt; }
        virtual void Render(Renderer2D& r, float alpha) { (void)r; (void)alpha; }
        virtual void OnShutdown() {}
    };

    int Run(const AppConfig& cfg, IGame& game);
}