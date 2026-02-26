#pragma once
#include <string>
#include <vector>
#include <cmath>

#include <glm/vec2.hpp>
#include "Platform/Sdl.h"
#include "Physics/Box2D.h"

class b2Body;

namespace my2d
{
    struct TagComponent
    {
        std::string tag;
    };

    struct TransformComponent
    {
        glm::vec2 position{ 0.0f, 0.0f };  // world units (treat as pixels for now)
        float rotationDeg = 0.0f;
        glm::vec2 scale{ 1.0f, 1.0f };
    };

    struct SpriteRendererComponent
    {
        std::string texturePath;          // asset key/path
        glm::vec2 size{ 64.0f, 64.0f };   // world size (pixels)
        SDL_Color tint{ 255, 255, 255, 255 };
        int layer = 0;

        bool useSourceRect = false;
        SDL_Rect sourceRect{ 0, 0, 0, 0 };

        SDL_RendererFlip flip = SDL_FLIP_NONE;
    };

    struct Tileset
    {
        std::string texturePath; // atlas texture
        int tileWidth = 32;
        int tileHeight = 32;

        // Atlas layout
        int columns = 1;   // tiles per row in atlas
        int margin = 0;    // pixels
        int spacing = 0;   // pixels
    };

    struct TileLayer
    {
        std::string name = "Layer";
        int layer = 0; // render order shared with SpriteRendererComponent::layer
        bool visible = true;
        SDL_Color tint{ 255, 255, 255, 255 };

        // size = width*height, values are 0-based tile indices into the atlas, -1 = empty
        std::vector<int> tiles;
    };

    struct TilemapComponent
    {
        int width = 0;
        int height = 0;

        int tileWidth = 32;
        int tileHeight = 32;

        Tileset tileset;
        std::vector<TileLayer> layers;

        // Populated at runtime from window size for culling (kept here to avoid threading more params)
        int viewportW = 0;
        int viewportH = 0;
    };


    struct TilemapColliderComponent
    {
        int collisionLayerIndex = 0;
        float friction = 0.8f;
        float restitution = 0.0f;
        bool isSensor = false;

        // runtime (Box2D 3.x uses ids/handles)
        std::vector<b2BodyId> runtimeBodies;
    };


    enum class BodyType2D
    {
        Static = 0,
        Kinematic,
        Dynamic
    };

    struct RigidBody2DComponent
    {
        BodyType2D type = BodyType2D::Dynamic;

        bool fixedRotation = true;
        bool enableSleep = true;
        bool isBullet = false;

        float gravityScale = 1.0f;
        float linearDamping = 0.0f;
        float angularDamping = 0.0f;

        // runtime
        b2BodyId bodyId = b2_nullBodyId;
    };

    struct BoxCollider2DComponent
    {
        // PIXELS
        glm::vec2 size{ 32.0f, 32.0f };
        glm::vec2 offset{ 0.0f, 0.0f };

        float density = 1.0f;
        float friction = 0.2f;
        float restitution = 0.0f;
        bool isSensor = false;

        // runtime
        b2ShapeId shapeId = b2_nullShapeId;
    };
}