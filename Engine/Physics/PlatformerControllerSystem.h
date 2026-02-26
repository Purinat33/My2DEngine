#pragma once
#include "Scene/Scene.h"
#include "Core/Engine.h"

namespace my2d
{
	// Call from OnFixedUpdate (fixedDt is seconds)
	void PlatformerController_FixedUpdate(Engine& engine, Scene& scene, float fixedDt);
}