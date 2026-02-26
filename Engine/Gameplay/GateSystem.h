#pragma once
namespace my2d
{
    class Engine;
    class Scene;

    // Re-evaluate all gates based on the current WorldState.
    void GateSystem_Update(Engine& engine, Scene& scene);
}