#include "framework.h"
#include <spdlog/spdlog.h>

class TestGame : public my2d::App
{
public:
    bool OnInit(my2d::Engine&) override
    {
        spdlog::info("TestGame OnInit");
        return true;
    }

    void OnUpdate(my2d::Engine& engine, double) override
    {
        // ESC to quit
        if (engine.GetInput().WasKeyPressed(SDL_SCANCODE_ESCAPE))
            engine.RequestQuit();
    }

    void OnRender(my2d::Engine& engine) override
    {
        // Nothing yet (engine clears the screen)
        (void)engine;
    }
};

int main()
{
    my2d::Engine engine;
    my2d::EngineConfig cfg;
    cfg.windowTitle = "My2DEngine - Game";

    TestGame game;
    return engine.Run(cfg, game);
}