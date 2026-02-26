#include "framework.h"

#include <spdlog/spdlog.h>
#include <filesystem>
#include <cmath>

#include "Gameplay/RoomManager.h"
#include "Gameplay/SaveGame.h"
#include "Gameplay/ProgressionSystem.h"

#include "Scene/SceneSerializer.h"
#include "Scene/Components.h"

#include "Physics/PlatformerControllerSystem.h"
#include "Physics/PhysicsLayers.h"

#include "Gameplay/CombatSystem.h"
#include "Physics/PhysicsSystem.h"

class MyGame : public my2d::App
{
public:
    bool OnInit(my2d::Engine& engine) override
    {
        LoadSave(engine);

        EnsureDefaultRoomsExist(engine);

        if (!m_rooms.LoadRoom(engine, m_startRoom, m_startSpawn))
            return false;

        return true;
    }

    void OnUpdate(my2d::Engine& engine, double dt) override
    {
        // Progression (pickups) against current room + player
        my2d::Progression_Update(engine, m_rooms.GetScene(), m_rooms.GetPlayer());

        // Door triggers / room transitions
        m_rooms.Update(engine, (float)dt);

        // Save/load hotkeys
        if (engine.GetInput().WasKeyPressed(SDL_SCANCODE_F5))
            Save(engine);

        if (engine.GetInput().WasKeyPressed(SDL_SCANCODE_F9))
        {
            LoadSave(engine);
            // Reload current room (or fall back to start)
            const std::string room = m_rooms.CurrentRoom().empty() ? m_startRoom : m_rooms.CurrentRoom();
            m_rooms.LoadRoom(engine, room, m_startSpawn);
        }

        if (engine.GetInput().WasKeyPressed(SDL_SCANCODE_F8))
        {
            engine.GetWorldState() = my2d::WorldState{};
            spdlog::info("Cleared world state");
            m_rooms.LoadRoom(engine, m_startRoom, m_startSpawn);
        }

        if (engine.GetInput().WasKeyPressed(SDL_SCANCODE_ESCAPE))
            engine.RequestQuit();

        // Camera follows current player
        auto& cam = engine.GetRenderer2D().GetCamera();
        cam.SetPosition(m_rooms.GetPlayer().Get<my2d::TransformComponent>().position);
    }

    void OnFixedUpdate(my2d::Engine& engine, double fixedDt) override
    {
        my2d::PlatformerController_FixedUpdate(engine, m_rooms.GetScene(), (float)fixedDt);
        my2d::Combat_PrePhysics(engine, m_rooms.GetScene(), (float)fixedDt);
    }

    void OnPostFixedUpdate(my2d::Engine& engine, double fixedDt) override
    {
        my2d::Combat_PostPhysics(engine, m_rooms.GetScene(), (float)fixedDt);
        my2d::Physics_SyncTransforms(m_rooms.GetScene(), engine.PixelsPerMeter());
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

        m_rooms.GetScene().OnRender(engine);
    }

private:
    void LoadSave(my2d::Engine& engine)
    {
        my2d::SaveGame save;
        if (my2d::SaveGame::LoadFromFile(m_savePath, save))
        {
            engine.GetWorldState() = save.world;
            spdlog::info("Loaded save from '{}'", m_savePath);
        }
        else
        {
            spdlog::info("No save found at '{}' (starting fresh)", m_savePath);
        }
    }

    void Save(my2d::Engine& engine)
    {
        my2d::SaveGame save;
        save.world = engine.GetWorldState();
        if (save.SaveToFile(m_savePath))
            spdlog::info("Saved to '{}'", m_savePath);
        else
            spdlog::error("Failed to save to '{}'", m_savePath);
    }

    static void FillTestTileLayer(my2d::TilemapComponent& tm, my2d::TileLayer& base)
    {
        base.tiles.assign(tm.width * tm.height, -1);

        for (int y = 0; y < tm.height; ++y)
        {
            for (int x = 0; x < tm.width; ++x)
            {
                const int idx = y * tm.width + x;

                // ground
                if (y == tm.height - 1) base.tiles[idx] = 1;

                // dots
                else if ((x + y) % 17 == 0) base.tiles[idx] = 5;
            }
        }

        // optional demo slopes (indices must match your tileset)
        // Put a small slope ramp near the left
        // "/" at tile index 2, "\" at tile index 3 (as you’ve been using)
        // This just paints a diagonal staircase of slope tiles for demo.
        const int startX = 8;
        const int baseY = tm.height - 2;
        for (int i = 0; i < 6; ++i)
        {
            int x = startX + i;
            int y = baseY - i;
            if (x >= 0 && x < tm.width && y >= 0 && y < tm.height)
                base.tiles[y * tm.width + x] = 2; // slopeUpRightTiles
        }
    }

    void EnsureDefaultRoomsExist(my2d::Engine& engine)
    {
        namespace fs = std::filesystem;

        fs::path scenesDir = fs::path(engine.ContentRoot()) / "Scenes";
        fs::create_directories(scenesDir);

        fs::path roomAPath = scenesDir / "room_start.scene.json";
        fs::path roomBPath = scenesDir / "room_b.scene.json";

        if (fs::exists(roomAPath) && fs::exists(roomBPath))
            return;

        spdlog::info("Scenes not found; generating default rooms into {}", scenesDir.string());

        // ---- Room A ----
        {
            my2d::Scene s;

            // Tilemap
            auto tmEnt = s.CreateEntity("Tilemap");
            tmEnt.Get<my2d::TransformComponent>().position = { -400.0f, -200.0f };

            auto& tm = tmEnt.Add<my2d::TilemapComponent>();
            tm.width = 50;
            tm.height = 25;
            tm.tileWidth = 32;
            tm.tileHeight = 32;

            tm.tileset.texturePath = "tiles.png";
            tm.tileset.tileWidth = 32;
            tm.tileset.tileHeight = 32;
            tm.tileset.columns = 8;

            my2d::TileLayer base;
            base.name = "Base";
            base.layer = -10;
            FillTestTileLayer(tm, base);
            tm.layers.push_back(std::move(base));

            auto& tmc = tmEnt.Add<my2d::TilemapColliderComponent>();
            tmc.collisionLayerIndex = 0;
            tmc.friction = 0.9f;
            tmc.slopeUpRightTiles = { 2 };
            tmc.slopeUpLeftTiles = { 3 };
            tmc.slopeFriction = 0.02f;

            // Spawn
            auto sp = s.CreateEntity("SpawnStart");
            sp.Get<my2d::TransformComponent>().position = { 0.0f, -200.0f };
            sp.Add<my2d::PlayerSpawnComponent>().name = "start";

            // Door to room B
            auto d = s.CreateEntity("DoorToRoomB");
            d.Get<my2d::TransformComponent>().position = { 320.0f, -232.0f };
            auto& door = d.Add<my2d::DoorComponent>();
            door.targetScene = "Scenes/room_b.scene.json";
            door.targetSpawn = "from_left";
            door.triggerSize = { 48.0f, 96.0f };
            door.requireInteract = true;
            door.interactKey = SDL_SCANCODE_W;
            door.autoTrigger = false;

            // Dash pickup (optional)
            auto pickup = s.CreateEntity("DashPickup");
            pickup.Get<my2d::TransformComponent>().position = { 200.0f, -220.0f };
            auto& pspr = pickup.Add<my2d::SpriteRendererComponent>();
            pspr.texturePath = "test.png";
            pspr.size = { 32.0f, 32.0f };
            pspr.layer = 1;

            auto& pf = pickup.Add<my2d::PersistentFlagComponent>();
            pf.flag = "pickup_dash";

            auto& gp = pickup.Add<my2d::GrantProgressionComponent>();
            gp.setFlag = "pickup_dash";
            gp.unlockAbility = true;
            gp.ability = my2d::AbilityId::Dash;
            gp.radiusPx = 32.0f;

            // Gate that opens when Dash is unlocked (optional)
            auto gate = s.CreateEntity("DashGate");
            gate.Get<my2d::TransformComponent>().position = { 360.0f, -232.0f };

            auto& gspr = gate.Add<my2d::SpriteRendererComponent>();
            gspr.texturePath = "test.png";
            gspr.size = { 32.0f, 96.0f };
            gspr.layer = 1;

            auto& g = gate.Add<my2d::GateComponent>();
            g.requireAllAbilities = { my2d::AbilityId::Dash };
            g.openWhenSatisfied = true;
            g.openBehavior = my2d::GateOpenBehavior::DisableCollider;
            g.overrideTint = true;

            auto& grb = gate.Add<my2d::RigidBody2DComponent>();
            grb.type = my2d::BodyType2D::Static;

            auto& gbc = gate.Add<my2d::BoxCollider2DComponent>();
            gbc.size = gspr.size;
            gbc.categoryBits = my2d::PhysicsLayers::Environment;
            gbc.maskBits = my2d::PhysicsLayers::Player;

            // --- Simple enemy dummy ---
            auto enemy = s.CreateEntity("DummyEnemy");
            enemy.Get<my2d::TransformComponent>().position = { 140.0f, -200.0f };

            auto& espr = enemy.Add<my2d::SpriteRendererComponent>();
            espr.texturePath = "test.png";
            espr.size = { 48.0f, 48.0f };
            espr.layer = 0;
            espr.tint = SDL_Color{ 255, 200, 80, 255 };

            auto& eteam = enemy.Add<my2d::TeamComponent>();
            eteam.team = my2d::Team::Enemy;

            auto& ehp = enemy.Add<my2d::HealthComponent>();
            ehp.maxHp = 3;
            ehp.hp = 3;

            enemy.Add<my2d::HurtboxComponent>();

            auto& erb = enemy.Add<my2d::RigidBody2DComponent>();
            erb.type = my2d::BodyType2D::Dynamic;
            erb.fixedRotation = true;
            erb.enableSleep = false;
            erb.gravityScale = 0.0f; // stay in place

            auto& ebc = enemy.Add<my2d::BoxCollider2DComponent>();
            ebc.size = espr.size;
            ebc.density = 1.0f;
            ebc.friction = 0.0f;
            ebc.categoryBits = my2d::PhysicsLayers::Enemy;
            ebc.maskBits = my2d::PhysicsLayers::Player | my2d::PhysicsLayers::Hitbox | my2d::PhysicsLayers::Environment;

            my2d::SceneSerializer::SaveToFile(s, roomAPath.string());
        }

        // ---- Room B ----
        {
            my2d::Scene s;

            auto tmEnt = s.CreateEntity("Tilemap");
            tmEnt.Get<my2d::TransformComponent>().position = { -400.0f, -200.0f };

            auto& tm = tmEnt.Add<my2d::TilemapComponent>();
            tm.width = 50;
            tm.height = 25;
            tm.tileWidth = 32;
            tm.tileHeight = 32;

            tm.tileset.texturePath = "tiles.png";
            tm.tileset.tileWidth = 32;
            tm.tileset.tileHeight = 32;
            tm.tileset.columns = 8;

            my2d::TileLayer base;
            base.name = "Base";
            base.layer = -10;
            FillTestTileLayer(tm, base);
            tm.layers.push_back(std::move(base));

            auto& tmc = tmEnt.Add<my2d::TilemapColliderComponent>();
            tmc.collisionLayerIndex = 0;
            tmc.friction = 0.9f;
            tmc.slopeUpRightTiles = { 2 };
            tmc.slopeUpLeftTiles = { 3 };
            tmc.slopeFriction = 0.02f;

            auto sp = s.CreateEntity("SpawnFromLeft");
            sp.Get<my2d::TransformComponent>().position = { -250.0f, -200.0f };
            sp.Add<my2d::PlayerSpawnComponent>().name = "from_left";

            // Door back to A
            auto d = s.CreateEntity("DoorToRoomA");
            d.Get<my2d::TransformComponent>().position = { -300.0f, -232.0f };
            auto& door = d.Add<my2d::DoorComponent>();
            door.targetScene = "Scenes/room_start.scene.json";
            door.targetSpawn = "start";
            door.triggerSize = { 48.0f, 96.0f };
            door.requireInteract = true;
            door.interactKey = SDL_SCANCODE_W;
            door.autoTrigger = false;

            my2d::SceneSerializer::SaveToFile(s, roomBPath.string());
        }
    }

private:
    my2d::RoomManager m_rooms;

    std::string m_savePath = "savegame.json";
    std::string m_startRoom = "Scenes/room_start.scene.json";
    std::string m_startSpawn = "start";
};

int main()
{
    my2d::Engine engine;
    my2d::EngineConfig cfg;
    cfg.windowTitle = "My2DEngine - Rooms";
    cfg.contentRoot = "Game/Content";

    MyGame game;
    return engine.Run(cfg, game);
}