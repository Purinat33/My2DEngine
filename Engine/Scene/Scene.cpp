#include "pch.h"
#include "Scene/Scene.h"
#include "Core/Engine.h"
#include "Assets/AssetManager.h"
#include "Renderer/Renderer2D.h"
#include "Renderer/Texture2D.h"

#include <algorithm>
#include <vector>

namespace my2d
{
    Entity Scene::CreateEntity(const std::string& name)
    {
        entt::entity e = m_registry.create();
        Entity entity(e, this);

        entity.Add<TransformComponent>();
        entity.Add<TagComponent>(TagComponent{ name });

        return entity;
    }

    void Scene::DestroyEntity(Entity e)
    {
        if (e)
            m_registry.destroy(e.Handle());
    }

    void Scene::OnUpdate(Engine&, double)
    {
        // Hook for scripts/systems later.
    }

    void Scene::OnRender(Engine& engine)
    {
        auto view = m_registry.view<TransformComponent, SpriteRendererComponent>();

        struct DrawItem
        {
            int layer;
            entt::entity entity;
        };

        std::vector<DrawItem> items;
        items.reserve(view.size_hint());

        for (auto e : view)
        {
            const auto& sprite = view.get<SpriteRendererComponent>(e);
            items.push_back({ sprite.layer, e });
        }

        std::sort(items.begin(), items.end(), [](const DrawItem& a, const DrawItem& b)
            {
                return a.layer < b.layer;
            });

        for (const auto& item : items)
        {
            auto& tc = view.get<TransformComponent>(item.entity);
            auto& sc = view.get<SpriteRendererComponent>(item.entity);

            if (sc.texturePath.empty())
                continue;

            auto tex = engine.GetAssets().GetTexture(sc.texturePath);
            if (!tex)
                continue;

            const SDL_Rect* src = sc.useSourceRect ? &sc.sourceRect : nullptr;

            engine.GetRenderer2D().DrawTexture(
                *tex,
                tc.position,
                { sc.size.x * tc.scale.x, sc.size.y * tc.scale.y },
                src,
                tc.rotationDeg,
                sc.flip,
                sc.tint
            );
        }
    }
}