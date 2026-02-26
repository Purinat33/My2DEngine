#pragma once
#include <entt/entt.hpp>
#include <string>

#include "Scene/Entity.h"
#include "Scene/Components.h"

namespace my2d
{
    class Engine;

    class Scene
    {
    public:
        Entity CreateEntity(const std::string& name = "Entity");
        void DestroyEntity(Entity e);

        void OnUpdate(Engine& engine, double dt);
        void OnRender(Engine& engine);

        entt::registry& Registry() { return m_registry; }

    private:
        entt::registry m_registry;
    };

    // ---- Entity template implementations (need Scene definition) ----
    template<typename T, typename... Args>
    T& Entity::Add(Args&&... args)
    {
        return m_scene->Registry().emplace<T>(m_handle, std::forward<Args>(args)...);
    }

    template<typename T>
    bool Entity::Has() const
    {
        return m_scene->Registry().all_of<T>(m_handle);
    }

    template<typename T>
    T& Entity::Get()
    {
        return m_scene->Registry().get<T>(m_handle);
    }

    template<typename T>
    void Entity::Remove()
    {
        m_scene->Registry().remove<T>(m_handle);
    }
}