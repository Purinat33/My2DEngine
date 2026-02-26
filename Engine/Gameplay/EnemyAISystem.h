#pragma once
namespace my2d
{
    class Engine;
    class Scene;
    class Entity;

    // Call in fixed update BEFORE Combat_PrePhysics
    void EnemyAI_FixedUpdate(Engine& engine, Scene& scene, float fixedDt, Entity player);
}