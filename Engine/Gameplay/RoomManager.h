#pragma once
#include <memory>
#include <string>

#include "Scene/Scene.h"
#include "Scene/Entity.h"

namespace my2d
{
    class Engine;

    class RoomManager
    {
    public:
        bool LoadRoom(Engine& engine, std::string sceneRelPath, std::string spawnName);
        void Update(Engine& engine, float dt);

        Scene& GetScene() { return *m_scene; }
        const Scene& GetScene() const { return *m_scene; }

        Entity GetPlayer() const { return m_player; }
        const std::string& CurrentRoom() const { return m_currentRoom; }

        // If you want game code to spawn player differently later:
        void SetPlayer(Entity e) { m_player = e; }

    private:
        std::string ResolveScenePath(const Engine& engine, const std::string& rel) const;

        Entity FindSpawn(const std::string& name);
        void PlacePlayerAtSpawn(Engine& engine, const std::string& spawnName);
        bool PlayerOverlapsDoor(const TransformComponent& playerT, const BoxCollider2DComponent* playerBox,
            const TransformComponent& doorT, const DoorComponent& door) const;

    private:
        std::unique_ptr<Scene> m_scene;
        Entity m_player;
        std::string m_currentRoom;

        float m_transitionLock = 0.0f;
    };
}