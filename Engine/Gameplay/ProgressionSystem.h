#pragma once
#include "Core/Engine.h"
#include "Scene/Scene.h"
#include "Scene/Entity.h"

namespace my2d
{
	// Call once after you build/load a scene
	void Progression_ApplyPersistence(Engine& engine, Scene& scene);

	// Call every update (or fixed update). Uses simple distance check vs player.
	void Progression_Update(Engine& engine, Scene& scene, Entity player);
}