#include "framework.h"
#include "Scene/Scene.h"
#include <spdlog/spdlog.h>

class MyGame : public my2d::App
{
public:
    bool OnInit(my2d::Engine& engine) override
    {
        m_scene = std::make_unique<my2d::Scene>();

        auto tmEnt = m_scene->CreateEntity("Tilemap");
        auto& tmT = tmEnt.Get<my2d::TransformComponent>();
        tmT.position = { -400.0f, -200.0f }; // move map so origin isn’t centered

        auto& tm = tmEnt.Add<my2d::TilemapComponent>();
        tm.width = 50;
        tm.height = 25;
        tm.tileWidth = 32;
        tm.tileHeight = 32;

        tm.tileset.texturePath = "tiles.png";
        tm.tileset.tileWidth = 32;
        tm.tileset.tileHeight = 32;
        tm.tileset.columns = 8;
        tm.tileset.margin = 0;
        tm.tileset.spacing = 0;

        my2d::TileLayer base;
        base.name = "Base";
        base.layer = -10; // behind sprites
        base.tiles.assign(tm.width * tm.height, -1);

        // simple pattern
        for (int y = 0; y < tm.height; ++y)
        {
            for (int x = 0; x < tm.width; ++x)
            {
                const int idx = y * tm.width + x;
                if (y == tm.height - 1) base.tiles[idx] = 1;         // ground row
                else if ((x + y) % 17 == 0) base.tiles[idx] = 5;     // dots
            }
        }

        tm.layers.push_back(std::move(base));

        auto e = m_scene->CreateEntity("TestSprite");
        auto& t = e.Get<my2d::TransformComponent>();
        t.position = { 0.0f, 0.0f };

        auto& s = e.Add<my2d::SpriteRendererComponent>();
        s.texturePath = "test.png";
        s.size = { 128.0f, 128.0f };
        s.layer = 0;

        // Center camera at world origin
        engine.GetRenderer2D().GetCamera().SetPosition({ 0.0f, 0.0f });
        engine.GetRenderer2D().GetCamera().SetZoom(1.0f);

        return true;
    }

    void OnUpdate(my2d::Engine& engine, double dt) override
    {
        (void)dt;

        if (engine.GetInput().WasKeyPressed(SDL_SCANCODE_ESCAPE))
            engine.RequestQuit();

        // WASD moves camera
        auto& cam = engine.GetRenderer2D().GetCamera();
        glm::vec2 p = cam.Position();

        const float speed = 400.0f; // pixels/sec
        if (engine.GetInput().IsKeyDown(SDL_SCANCODE_A)) p.x -= speed * (float)dt;
        if (engine.GetInput().IsKeyDown(SDL_SCANCODE_D)) p.x += speed * (float)dt;
        if (engine.GetInput().IsKeyDown(SDL_SCANCODE_W)) p.y -= speed * (float)dt;
        if (engine.GetInput().IsKeyDown(SDL_SCANCODE_S)) p.y += speed * (float)dt;

        cam.SetPosition(p);
    }

    void OnRender(my2d::Engine& engine) override
    {
        m_scene->OnRender(engine);
    }

private:
    std::unique_ptr<my2d::Scene> m_scene;
};

int main()
{
    my2d::Engine engine;
    my2d::EngineConfig cfg;
    cfg.windowTitle = "My2DEngine - Scene Test";
    cfg.contentRoot = "Game/Content"; // <- uses AssetManager root

    MyGame game;
    return engine.Run(cfg, game);
}