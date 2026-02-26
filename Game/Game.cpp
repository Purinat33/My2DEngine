#include "framework.h"
#include "Scene/Scene.h"
#include <spdlog/spdlog.h>
#include "Physics/TilemapColliderBuilder.h"
#include "Physics/PhysicsSystem.h"


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
        auto& tmc = tmEnt.Add<my2d::TilemapColliderComponent>();
        tmc.collisionLayerIndex = 0; // whichever layer is your solid tiles
        tmc.friction = 0.9f;
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

        my2d::BuildTilemapColliders(engine.GetPhysics(), *m_scene, engine.PixelsPerMeter());
        

        m_player = m_scene->CreateEntity("Player");
        auto& t = m_player.Get<my2d::TransformComponent>();
        t.position = { 0.0f, -200.0f }; // start above the ground so you can see it fall

        auto& s = m_player.Add<my2d::SpriteRendererComponent>();
        s.texturePath = "test.png";
        s.size = { 64.0f, 64.0f };
        s.layer = 0;

        // Physics
        auto& rb = m_player.Add<my2d::RigidBody2DComponent>();
        rb.enableSleep = false;
        rb.type = my2d::BodyType2D::Dynamic;
        rb.fixedRotation = true;
        rb.gravityScale = 1.0f;

        auto& box = m_player.Add<my2d::BoxCollider2DComponent>();
        box.size = s.size;
        box.density = 1.0f;
        box.friction = 0.0f; // player shouldn't "stick" on walls

        my2d::Physics_CreateRuntime(*m_scene, engine.GetPhysics(), engine.PixelsPerMeter());

        // Center camera at world origin
        engine.GetRenderer2D().GetCamera().SetPosition({ 0.0f, 0.0f });
        engine.GetRenderer2D().GetCamera().SetZoom(1.0f);

        return true;
    }

    void OnUpdate(my2d::Engine& engine, double dt) override
    {
        (void)dt;
        my2d::Physics_SyncTransforms(*m_scene, engine.PixelsPerMeter());

        if (engine.GetInput().WasKeyPressed(SDL_SCANCODE_ESCAPE))
            engine.RequestQuit();

        // Camera follows player
        auto& cam = engine.GetRenderer2D().GetCamera();
        cam.SetPosition(m_player.Get<my2d::TransformComponent>().position);
    }

    void OnFixedUpdate(my2d::Engine& engine, double) override
    {
        my2d::Physics_CreateRuntime(*m_scene, engine.GetPhysics(), engine.PixelsPerMeter());
        if (!m_player) return;

        auto& rb = m_player.Get<my2d::RigidBody2DComponent>();
        static bool once = false;
        if (!once) { once = true; spdlog::info("Player body valid? {}", b2Body_IsValid(rb.bodyId)); }
        if (!b2Body_IsValid(rb.bodyId)) return;


        b2Vec2 v = b2Body_GetLinearVelocity(rb.bodyId);

        const float speedPx = 300.0f;                 // pixels/sec
        const float speed = speedPx / engine.PixelsPerMeter(); // meters/sec

        float desiredX = 0.0f;
        if (engine.GetInput().IsKeyDown(SDL_SCANCODE_A)) desiredX -= speed;
        if (engine.GetInput().IsKeyDown(SDL_SCANCODE_D)) desiredX += speed;

        v.x = desiredX;
        b2Body_SetLinearVelocity(rb.bodyId, v);
    }

    void OnRender(my2d::Engine& engine) override
    {
        if (engine.DrawPhysicsDebug())
        {
            engine.GetPhysicsDebugDraw().Draw(
                engine.GetSDLRenderer(),
                engine.GetPhysics().WorldId(),
                engine.PixelsPerMeter(),
                engine.GetRenderer2D().GetCamera()
            );
        }
        m_scene->OnRender(engine);
    }

private:
    std::unique_ptr<my2d::Scene> m_scene;
    my2d::Entity m_player;
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