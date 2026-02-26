#pragma once
#include "Physics/Box2D.h"
#include "Scene/Scene.h"

namespace my2d
{
	// Builds/refreshes static colliders for every entity that has:
	// TransformComponent + TilemapComponent + TilemapColliderComponent
	void BuildTilemapColliders(PhysicsWorld& physics, Scene& scene, float pixelsPerMeter);
}