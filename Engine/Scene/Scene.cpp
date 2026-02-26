#include "pch.h"
#include "Scene/Scene.h"
#include "Core/Engine.h"
#include "Assets/AssetManager.h"
#include "Renderer/Renderer2D.h"
#include "Renderer/Texture2D.h"
#include "Renderer/TilemapRenderer2D.h"

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
        auto spriteView = m_registry.view<TransformComponent, SpriteRendererComponent>();
        auto tilemapView = m_registry.view<TransformComponent, TilemapComponent>();

        // Determine layer range across tilemap layers + sprites
        bool hasAny = false;
        int minLayer = 0;
        int maxLayer = 0;

        for (auto e : spriteView)
        {
            const auto& s = spriteView.get<SpriteRendererComponent>(e);
            if (!hasAny) { minLayer = maxLayer = s.layer; hasAny = true; }
            else { minLayer = std::min(minLayer, s.layer); maxLayer = std::max(maxLayer, s.layer); }
        }

        for (auto e : tilemapView)
        {
            const auto& tm = tilemapView.get<TilemapComponent>(e);
            for (const auto& layer : tm.layers)
            {
                if (!hasAny) { minLayer = maxLayer = layer.layer; hasAny = true; }
                else { minLayer = std::min(minLayer, layer.layer); maxLayer = std::max(maxLayer, layer.layer); }
            }
        }

        if (!hasAny) return;

        TilemapRenderer2D tileRenderer;

        // Render by layer in ascending order (shared integer layer space)
        for (int L = minLayer; L <= maxLayer; ++L)
        {
            // Tilemaps for this layer
            for (auto e : tilemapView)
            {
                auto& tc = tilemapView.get<TransformComponent>(e);
                auto& tm = tilemapView.get<TilemapComponent>(e);

                
                for (const auto& layer : tm.layers)
                {
                    if (layer.layer != L) continue;
                    tileRenderer.DrawLayer(tm, tc, layer, engine.GetAssets(), engine.GetRenderer2D());
                }
            }

            // Sprites for this layer
            for (auto e : spriteView)
            {
                auto& tc = spriteView.get<TransformComponent>(e);
                auto& sc = spriteView.get<SpriteRendererComponent>(e);

                if (sc.layer != L) continue;
                if (sc.texturePath.empty()) continue;

                auto tex = engine.GetAssets().GetTexture(sc.texturePath);
                if (!tex) continue;

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
}