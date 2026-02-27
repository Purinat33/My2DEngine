#include "pch.h"
#include "Scene/Scene.h"
#include "Core/Engine.h"
#include "Assets/AssetManager.h"
#include "Renderer/Renderer2D.h"
#include "Renderer/Texture2D.h"
#include "Renderer/TilemapRenderer2D.h"
#include "Renderer/AnimationSystem.h"

#include <algorithm>
#include <vector>
#include <random>
#include <Renderer/SpriteAtlas.h>


namespace my2d
{
    static uint32_t s_salt = []()
        {
            std::random_device rd;
            return (uint32_t)rd();
        }();

    static uint32_t s_counter = 1;

    static uint64_t GenerateId()
    {
        // Upper 32 bits = run salt, lower 32 bits = incrementing counter
        return (uint64_t(s_salt) << 32) | uint64_t(s_counter++);
    }

    static void TrackLoadedId(uint64_t id)
    {
        const uint32_t hi = uint32_t(id >> 32);
        const uint32_t lo = uint32_t(id & 0xFFFFFFFFu);
        if (hi == s_salt)
            s_counter = std::max(s_counter, lo + 1);
    }

    Entity Scene::CreateEntity(const std::string& name)
    {
        return CreateEntityWithId(GenerateId(), name);
    }

    Entity Scene::CreateEntityWithId(uint64_t id, const std::string& name)
    {
        TrackLoadedId(id);

        entt::entity e = m_registry.create();
        Entity entity(e, this);

        entity.Add<IdComponent>(IdComponent{ id });
        entity.Add<TransformComponent>();
        entity.Add<TagComponent>(TagComponent{ name });

        return entity;
    }

    Entity Scene::FindEntityById(uint64_t id)
    {
        auto view = m_registry.view<IdComponent>();
        for (auto e : view)
        {
            if (view.get<IdComponent>(e).id == id)
                return Entity(e, this);
        }
        return {};
    }

    void Scene::DestroyEntity(Entity e)
    {
        if (e)
            m_registry.destroy(e.Handle());
    }

    void Scene::OnUpdate(Engine& engine, double dt)
    {
        AnimationSystem_Update(engine, *this, (float)dt);
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

                // Pivot/offset drawing
                const glm::vec2 worldSize = { sc.size.x * tc.scale.x, sc.size.y * tc.scale.y };
                const glm::vec2 pivotScaled = { sc.pivot.x * worldSize.x, sc.pivot.y * worldSize.y };
                const glm::vec2 offsetScaled = { sc.offset.x * tc.scale.x, sc.offset.y * tc.scale.y };
                const glm::vec2 drawPos = tc.position + offsetScaled - pivotScaled;

                const SDL_Rect* src = nullptr;
                SDL_Rect atlasRect{};

                // Atlas mode
                if (!sc.atlasPath.empty() && !sc.regionName.empty())
                {
                    auto atlas = engine.GetAssets().GetAtlas(sc.atlasPath);
                    if (!atlas) continue;

                    const SpriteRegion* region = atlas->GetRegion(sc.regionName);
                    if (!region || !region->texture) continue;

                    atlasRect = region->rect;
                    src = &atlasRect;

                    engine.GetRenderer2D().DrawTexture(
                        *region->texture,
                        drawPos,
                        worldSize,
                        src,
                        tc.rotationDeg,
                        sc.flip,
                        sc.tint
                    );
                }
                else
                {
                    // Legacy texturePath mode
                    if (sc.texturePath.empty()) continue;

                    auto tex = engine.GetAssets().GetTexture(sc.texturePath);
                    if (!tex) continue;

                    src = sc.useSourceRect ? &sc.sourceRect : nullptr;

                    engine.GetRenderer2D().DrawTexture(
                        *tex,
                        drawPos,
                        worldSize,
                        src,
                        tc.rotationDeg,
                        sc.flip,
                        sc.tint
                    );
                }
            }
        }
    }
}