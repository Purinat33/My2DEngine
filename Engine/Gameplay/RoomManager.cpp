#include "pch.h"
#include "Gameplay/RoomManager.h"

#include "Core/Engine.h"
#include "Scene/SceneSerializer.h"
#include "Scene/Components.h"

#include "Gameplay/ProgressionSystem.h"
#include "Gameplay/GateSystem.h"

#include "Physics/TilemapColliderBuilder.h"
#include "Physics/PhysicsSystem.h"

#include <algorithm>
#include <spdlog/spdlog.h>

namespace my2d
{
    static std::string JoinPath(const std::string& a, const std::string& b)
    {
        if (a.empty()) return b;
        if (a.back() == '/' || a.back() == '\\') return a + b;
        return a + "/" + b;
    }

    std::string RoomManager::ResolveScenePath(const Engine& engine, const std::string& rel) const
    {
        // rel like "Scenes/room_a.scene.json"
        if (rel.find(':') != std::string::npos) // crude "absolute on Windows"
            return rel;

        // contentRoot like "Game/Content"
        return JoinPath(engine.ContentRoot(), rel);
    }

    Entity RoomManager::FindSpawn(const std::string& name)
    {
        if (!m_scene) return {};
        auto& reg = m_scene->Registry();

        auto view = reg.view<TransformComponent, PlayerSpawnComponent>();
        for (auto e : view)
        {
            auto& sp = view.get<PlayerSpawnComponent>(e);
            if (sp.name == name)
                return Entity(e, m_scene.get());
        }
        return {};
    }

    void RoomManager::PlacePlayerAtSpawn(Engine& engine, const std::string& spawnName)
    {
        if (!m_scene || !m_player) return;

        Entity sp = FindSpawn(spawnName);
        if (!sp)
        {
            spdlog::warn("Spawn '{}' not found; using (0,0)", spawnName);
            m_player.Get<TransformComponent>().position = { 0.0f, 0.0f };
        }
        else
        {
            m_player.Get<TransformComponent>().position = sp.Get<TransformComponent>().position;
        }

        // Force physics body recreation at the new position
        if (m_player.Has<RigidBody2DComponent>())
        {
            auto& rb = m_player.Get<RigidBody2DComponent>();
            if (b2Body_IsValid(rb.bodyId))
                b2DestroyBody(rb.bodyId);
            rb.bodyId = b2_nullBodyId;
        }

        Physics_CreateRuntime(*m_scene, engine.GetPhysics(), engine.PixelsPerMeter());
    }

    bool RoomManager::PlayerOverlapsDoor(const TransformComponent& playerT, const BoxCollider2DComponent* playerBox,
        const TransformComponent& doorT, const DoorComponent& door) const
    {
        // Player AABB: use collider size if available, else assume 32x64
        glm::vec2 pSize = playerBox ? playerBox->size : glm::vec2{ 32.0f, 64.0f };

        const glm::vec2 pMin = playerT.position;
        const glm::vec2 pMax = playerT.position + pSize;

        const glm::vec2 dMin = doorT.position;
        const glm::vec2 dMax = doorT.position + door.triggerSize;

        const bool overlap =
            (pMin.x < dMax.x && pMax.x > dMin.x) &&
            (pMin.y < dMax.y && pMax.y > dMin.y);

        return overlap;
    }

    bool RoomManager::LoadRoom(Engine& engine, const std::string& sceneRelPath, const std::string& spawnName)
    {
        const std::string fullPath = ResolveScenePath(engine, sceneRelPath);

        engine.ResetPhysicsWorld();

        m_scene = std::make_unique<Scene>();
        if (!SceneSerializer::LoadFromFile(*m_scene, fullPath))
        {
            spdlog::error("Failed to load scene '{}'", fullPath);
            m_scene.reset();
            return false;
        }

        // Apply global state
        Progression_ApplyPersistence(engine, *m_scene);
        GateSystem_Update(engine, *m_scene);

        // Build tile colliders + runtime bodies
        BuildTilemapColliders(engine.GetPhysics(), *m_scene, engine.PixelsPerMeter());
        Physics_CreateRuntime(*m_scene, engine.GetPhysics(), engine.PixelsPerMeter());

        // Find existing player in scene or spawn a default one
        {
            auto& reg = m_scene->Registry();
            Entity foundPlayer{};

            auto view = reg.view<TransformComponent, RigidBody2DComponent, BoxCollider2DComponent, PlatformerControllerComponent>();
            for (auto e : view)
            {
                foundPlayer = Entity(e, m_scene.get());
                break;
            }

            if (foundPlayer)
            {
                m_player = foundPlayer;
            }
            else
            {
                // Minimal default player (replace later with prefab)
                m_player = m_scene->CreateEntity("Player");

                auto& t = m_player.Get<TransformComponent>();
                t.position = { 0.0f, 0.0f };

                auto& spr = m_player.Add<SpriteRendererComponent>();
                spr.texturePath = "test.png";
                spr.size = { 64.0f, 64.0f };
                spr.layer = 0;

                auto& rb = m_player.Add<RigidBody2DComponent>();
                rb.enableSleep = false;
                rb.type = BodyType2D::Dynamic;
                rb.fixedRotation = true;

                auto& bc = m_player.Add<BoxCollider2DComponent>();
                bc.size = spr.size;
                bc.friction = 0.0f;

                auto& pc = m_player.Add<PlatformerControllerComponent>();
                pc.moveSpeedPx = 320.0f;

                auto& team = m_player.Add<TeamComponent>();
                team.team = Team::Player;

                auto& hp = m_player.Add<HealthComponent>();
                hp.maxHp = 5;
                hp.hp = 5;

                m_player.Add<HurtboxComponent>();

                auto& atk = m_player.Add<MeleeAttackComponent>();
                atk.attackKey = SDL_SCANCODE_J;
                atk.damage = 1;
                atk.activeTime = 0.10f;
                atk.cooldown = 0.25f;
                atk.hitboxSizePx = { 48.0f, 28.0f };
                atk.hitboxOffsetPx = { 52.0f, 0.0f };
                atk.targetMaskBits = my2d::PhysicsLayers::Enemy;

                Physics_CreateRuntime(*m_scene, engine.GetPhysics(), engine.PixelsPerMeter());
            }
        }

        PlacePlayerAtSpawn(engine, spawnName);

        m_currentRoom = sceneRelPath;
        m_transitionLock = 0.2f;
        return true;
    }

    void RoomManager::Update(Engine& engine, float dt)
    {
        if (!m_scene || !m_player) return;

        m_transitionLock = std::max(0.0f, m_transitionLock - dt);

        // Update gate state (switches/flags can affect them)
        GateSystem_Update(engine, *m_scene);

        auto& reg = m_scene->Registry();

        auto& playerT = m_player.Get<TransformComponent>();
        BoxCollider2DComponent* playerBox = nullptr;
        if (m_player.Has<BoxCollider2DComponent>())
            playerBox = &m_player.Get<BoxCollider2DComponent>();

        auto view = reg.view<TransformComponent, DoorComponent>();
        for (auto e : view)
        {
            auto& doorT = view.get<TransformComponent>(e);
            auto& door = view.get<DoorComponent>(e);

            door.cooldownTimer = std::max(0.0f, door.cooldownTimer - dt);

            if (m_transitionLock > 0.0f || door.cooldownTimer > 0.0f)
                continue;

            if (!PlayerOverlapsDoor(playerT, playerBox, doorT, door))
                continue;

            bool trigger = false;

            if (door.autoTrigger)
                trigger = true;
            else if (!door.requireInteract)
                trigger = true;
            else if (engine.GetInput().WasKeyPressed(door.interactKey))
                trigger = true;

            if (trigger)
            {
                door.cooldownTimer = 0.3f;
                LoadRoom(engine, door.targetScene, door.targetSpawn);
                return;
            }
        }
    }
}