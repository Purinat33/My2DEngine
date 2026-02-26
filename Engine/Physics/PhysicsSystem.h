#pragma once
#include "Physics/PhysicsWorld.h"
#include "Scene/Scene.h"

namespace my2d
{
	// Create missing runtime bodies/shapes for entities with:
	// TransformComponent + RigidBody2DComponent + BoxCollider2DComponent
	void Physics_CreateRuntime(Scene& scene, PhysicsWorld& physics, float pixelsPerMeter);

	// Pull body transforms back into TransformComponent for rendering
	void Physics_SyncTransforms(Scene& scene, float pixelsPerMeter);
}