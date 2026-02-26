#pragma once
#include <entt/entt.hpp>

namespace my2d
{
    class Scene;

    class Entity
    {
    public:
        Entity() = default;
        Entity(entt::entity handle, Scene* scene) : m_handle(handle), m_scene(scene) {}

        operator bool() const { return m_handle != entt::null; }
        entt::entity Handle() const { return m_handle; }

        template<typename T, typename... Args>
        T& Add(Args&&... args);

        template<typename T>
        bool Has() const;

        template<typename T>
        T& Get();

        template<typename T>
        void Remove();

    private:
        entt::entity m_handle{ entt::null };
        Scene* m_scene = nullptr;
    };
}