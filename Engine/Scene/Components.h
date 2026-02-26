#pragma once
#include <string>
#include <vector>
#include <cmath>

#include <glm/vec2.hpp>
#include "Platform/Sdl.h"
#include "Physics/Box2D.h"

#include "Physics/PhysicsLayers.h"

#include "Gameplay/Ability.h"

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

        // Slopes: list tile indices (atlas indices) that represent slope tiles
        std::vector<int> slopeUpRightTiles; // "/" (low on left, high on right)
        std::vector<int> slopeUpLeftTiles;  // "\" (high on left, low on right)

        // usually very low so you slide down
        float slopeFriction = 0.05f;

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

        bool enabled = true;

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

        bool enabled = true;

        float density = 1.0f;
        float friction = 0.2f;
        float restitution = 0.0f;
        bool isSensor = false;

        // Collision filtering (Box2D 3.x uses 64-bit category/mask bits)
        // Defaults match the current player-vs-environment setup.
        uint64_t categoryBits = my2d::PhysicsLayers::Player;
        uint64_t maskBits = my2d::PhysicsLayers::Environment;

        // runtime
        b2ShapeId shapeId = b2_nullShapeId;
    };

    enum class GateOpenBehavior
    {
        DisableCollider,          // default: keep entity, remove collision
        DisableColliderAndHide,   // collision off + sprite alpha 0
        DestroyEntity             // rare hard requirement
    };

    struct GateComponent
    {
        // Conditions (optional). If all lists are empty => always satisfied.
        std::vector<AbilityId> requireAllAbilities;
        std::vector<AbilityId> requireAnyAbilities;

        std::vector<std::string> requireAllFlags;
        std::vector<std::string> requireAnyFlags;

        bool invert = false;

        // If true: satisfied => open. If false: satisfied => closed.
        bool openWhenSatisfied = true;

        GateOpenBehavior openBehavior = GateOpenBehavior::DisableCollider;

        // Visual feedback (optional)
        bool overrideTint = true;
        SDL_Color closedTint{ 255, 120, 120, 255 };
        SDL_Color openTint{ 120, 255, 120, 200 };

        bool hideWhenOpen = false;

        // runtime
        bool initialized = false;
        bool isOpen = false;
    };

    // Legacy
    // Remove entity when the world state contains the condition.
    // Used for gates/doors/blockers that should disappear when an ability/flag is acquired.
    struct RemoveIfHasAbilityComponent
    {
        AbilityId ability = AbilityId::Dash;
    };

    struct RemoveIfHasFlagComponent
    {
        std::string flag;
    };

    struct PlatformerControllerComponent
    {
        // movement (PIXELS/sec and PIXELS/sec^2)
        float moveSpeedPx = 300.0f;
        float accelPx = 2500.0f;
        float decelPx = 3000.0f;

        // jump
        float jumpSpeedPx = 650.0f;        // initial jump velocity (px/s)
        float coyoteTime = 0.10f;          // seconds after leaving ground you can still jump
        float jumpBufferTime = 0.12f;      // seconds before landing to buffer jump
        float groundCheckDistancePx = 6.0f;
        float groundRayInsetPx = 6.0f;     // inset from edges for left/right rays

        // keys
        SDL_Scancode left = SDL_SCANCODE_A;
        SDL_Scancode right = SDL_SCANCODE_D;
        SDL_Scancode jump = SDL_SCANCODE_SPACE;
        SDL_Scancode dash = SDL_SCANCODE_LSHIFT;

        // dash tuning
        float dashSpeedPx = 850.0f;
        float dashTime = 0.12f;
        float dashCooldown = 0.35f;

        // runtime
        int facing = 1;               // -1 left, +1 right
        bool isDashing = false;
        float dashTimer = 0.0f;
        float dashCooldownTimer = 0.0f;
        int dashDir = 1;

        float maxGroundSlopeDeg = 55.0f; // consider grounded up to this
        float maxJumpSlopeDeg = 5.0f;    // only allow jump on very flat ground
        bool jumpableGround = false;     // runtime

        // runtime
        bool grounded = false;
        float coyoteTimer = 0.0f;
        float jumpBufferTimer = 0.0f;
    };

    struct PersistentFlagComponent
    {
        // If this flag is already set in WorldState, entity is removed on scene start
        std::string flag;
    };

    struct GrantProgressionComponent
    {
        // On touch: set this flag (optional)
        std::string setFlag;

        // On touch: unlock ability (optional)
        bool unlockAbility = false;
        AbilityId ability = AbilityId::Dash;

        // simple distance check (pixels)
        float radiusPx = 24.0f;
    };

}