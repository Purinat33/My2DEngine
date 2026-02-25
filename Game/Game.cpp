#include <SDL2/SDL.h>
#include <glm/glm.hpp>
#include <box2d/box2d.h>
#include <spdlog/spdlog.h>
#include <nlohmann/json.hpp>

int main(int, char**)
{
    spdlog::info("Hello from Game");

    // --- Box2D 3.x (handle-based API) ---
    b2WorldDef worldDef = b2DefaultWorldDef();
    worldDef.gravity = { 0.0f, -9.8f };

    b2WorldId worldId = b2CreateWorld(&worldDef);

    // Step the world a few frames
    for (int i = 0; i < 60; ++i)
    {
        b2World_Step(worldId, 1.0f / 60.0f, 4);
    }

    b2DestroyWorld(worldId);
    worldId = b2_nullWorldId;

    // --- SDL sanity ---
    SDL_Init(SDL_INIT_VIDEO);
    SDL_Quit();

    return 0;
}