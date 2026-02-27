#include "pch.h"
#include "Renderer/AnimationSystem.h"

#include "Core/Engine.h"
#include "Scene/Scene.h"
#include "Scene/Components.h"
#include "Renderer/AnimationSet.h"
#include "Renderer/SpriteAtlas.h"

#include <cmath>

namespace my2d
{
    void AnimationSystem_Update(Engine& engine, Scene& scene, float dt)
    {
        auto& reg = scene.Registry();

        auto view = reg.view<SpriteRendererComponent, AnimatorComponent>();
        for (auto e : view)
        {
            auto& spr = view.get<SpriteRendererComponent>(e);
            auto& an = view.get<AnimatorComponent>(e);

            if (an.animSetPath.empty())
                continue;

            auto set = engine.GetAssets().GetAnimationSet(an.animSetPath);
            if (!set) continue;

            const AnimationClip* clip = set->GetClip(an.clip);
            if (!clip || clip->frames.empty()) continue;

            // advance time
            if (an.playing)
                an.time += dt * an.speed;

            const float fps = std::max(1.0f, clip->fps);
            const float frameTime = 1.0f / fps;

            int frameCount = (int)clip->frames.size();

            int idx = (int)std::floor(an.time / frameTime);
            if (clip->loop)
                idx = idx % frameCount;
            else
                idx = std::min(idx, frameCount - 1);

            an.frameIndex = idx;

            // write sprite selection
            spr.atlasPath = set->AtlasPath();
            spr.regionName = clip->frames[idx];
        }
    }
}