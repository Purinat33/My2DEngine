#pragma once
namespace my2d
{
    class Engine;
    class Scene;

    // Call BEFORE physics step: update timers, create/destroy hitbox shapes
    void Combat_PrePhysics(Engine& engine, Scene& scene, float fixedDt);

    // Call AFTER physics step: read sensor events and apply damage/knockback
    void Combat_PostPhysics(Engine& engine, Scene& scene, float fixedDt);
}