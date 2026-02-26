#pragma once
#include "Physics/Box2D.h"
#include "Platform/Sdl.h"
#include "Renderer/Camera2D.h"

namespace my2d
{
    class PhysicsDebugDraw
    {
    public:
        void Draw(SDL_Renderer* sdlRenderer, b2WorldId worldId, float pixelsPerMeter, const Camera2D& cam);
    };
}