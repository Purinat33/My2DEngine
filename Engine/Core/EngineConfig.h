#pragma once
#include <string>

namespace my2d
{
    struct EngineConfig
    {
        std::string windowTitle = "My2DEngine";
        int windowWidth = 1280;
        int windowHeight = 720;
        bool resizable = true;
        bool vsync = true;

        // Used by the engine loop for fixed updates (physics).
        double fixedDeltaSeconds = 1.0 / 60.0;

        float gravityX = 0.0f;
        float gravityY = 9.8f;          // y+ down (works fine with SDL coords)
        float pixelsPerMeter = 100.0f;  // conversion scale
        bool drawPhysicsDebug = true;

        std::string contentRoot = ""; // e.g. "$(SolutionDir)Game/Content" or "Game/Content"
    };
}