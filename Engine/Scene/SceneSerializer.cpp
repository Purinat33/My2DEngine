#include "pch.h"
#include "Scene/SceneSerializer.h"

#include "Scene/Scene.h"
#include "Scene/Components.h"

#include <nlohmann/json.hpp>
#include <fstream>

namespace my2d
{
    using json = nlohmann::json;

    // ---- helpers ----
    static json Vec2ToJson(const glm::vec2& v) { return json{ {"x", v.x}, {"y", v.y} }; }
    static glm::vec2 JsonToVec2(const json& j, glm::vec2 def = { 0,0 })
    {
        if (!j.is_object()) return def;
        return { j.value("x", def.x), j.value("y", def.y) };
    }

    static json ColorToJson(const SDL_Color& c) { return json{ {"r", c.r}, {"g", c.g}, {"b", c.b}, {"a", c.a} }; }
    static SDL_Color JsonToColor(const json& j, SDL_Color def = { 255,255,255,255 })
    {
        if (!j.is_object()) return def;
        SDL_Color c = def;
        c.r = (Uint8)j.value("r", (int)def.r);
        c.g = (Uint8)j.value("g", (int)def.g);
        c.b = (Uint8)j.value("b", (int)def.b);
        c.a = (Uint8)j.value("a", (int)def.a);
        return c;
    }

    static json RectToJson(const SDL_Rect& r) { return json{ {"x", r.x}, {"y", r.y}, {"w", r.w}, {"h", r.h} }; }
    static SDL_Rect JsonToRect(const json& j, SDL_Rect def = { 0,0,0,0 })
    {
        if (!j.is_object()) return def;
        SDL_Rect r = def;
        r.x = j.value("x", def.x);
        r.y = j.value("y", def.y);
        r.w = j.value("w", def.w);
        r.h = j.value("h", def.h);
        return r;
    }

    static const char* BodyTypeToString(BodyType2D t)
    {
        switch (t)
        {
        case BodyType2D::Static: return "Static";
        case BodyType2D::Kinematic: return "Kinematic";
        case BodyType2D::Dynamic: return "Dynamic";
        default: return "Dynamic";
        }
    }

    static BodyType2D BodyTypeFromString(const std::string& s, BodyType2D def)
    {
        if (s == "Static") return BodyType2D::Static;
        if (s == "Kinematic") return BodyType2D::Kinematic;
        if (s == "Dynamic") return BodyType2D::Dynamic;
        return def;
    }

    static const char* GateBehaviorToString(GateOpenBehavior b)
    {
        switch (b)
        {
        case GateOpenBehavior::DisableCollider: return "DisableCollider";
        case GateOpenBehavior::DisableColliderAndHide: return "DisableColliderAndHide";
        case GateOpenBehavior::DestroyEntity: return "DestroyEntity";
        default: return "DisableCollider";
        }
    }

    static GateOpenBehavior GateBehaviorFromString(const std::string& s, GateOpenBehavior def)
    {
        if (s == "DisableCollider") return GateOpenBehavior::DisableCollider;
        if (s == "DisableColliderAndHide") return GateOpenBehavior::DisableColliderAndHide;
        if (s == "DestroyEntity") return GateOpenBehavior::DestroyEntity;
        return def;
    }

    static json AbilityListToJson(const std::vector<AbilityId>& v)
    {
        json arr = json::array();
        for (auto a : v)
            arr.push_back(std::string(AbilityName(a)));
        return arr;
    }

    static std::vector<AbilityId> AbilityListFromJson(const json& j)
    {
        std::vector<AbilityId> out;
        if (!j.is_array()) return out;
        for (auto& it : j)
        {
            if (!it.is_string()) continue;
            AbilityId a{};
            if (AbilityFromName(it.get<std::string>(), a))
                out.push_back(a);
        }
        return out;
    }

    static void SavePlayerSpawn(json& e, const PlayerSpawnComponent& c)
    {
        e["PlayerSpawn"] = json{ {"name", c.name} };
    }

    static void SaveDoor(json& e, const DoorComponent& c)
    {
        e["Door"] = json{
            {"targetScene", c.targetScene},
            {"targetSpawn", c.targetSpawn},
            {"triggerSize", Vec2ToJson(c.triggerSize)},
            {"requireInteract", c.requireInteract},
            {"interactKey", (int)c.interactKey},
            {"autoTrigger", c.autoTrigger}
        };
    }

    // ---- component save ----
    static void SaveTransform(json& e, const TransformComponent& c)
    {
        e["Transform"] = json{
            {"position", Vec2ToJson(c.position)},
            {"rotationDeg", c.rotationDeg},
            {"scale", Vec2ToJson(c.scale)}
        };
    }

    static void SaveSprite(json& e, const SpriteRendererComponent& c)
    {
        e["SpriteRenderer"] = json{
            {"texturePath", c.texturePath},
            {"size", Vec2ToJson(c.size)},
            {"tint", ColorToJson(c.tint)},
            {"layer", c.layer},
            {"useSourceRect", c.useSourceRect},
            {"sourceRect", RectToJson(c.sourceRect)},
            {"flip", (int)c.flip}
        };
    }

    static void SaveTilemap(json& e, const TilemapComponent& tm)
    {
        json j;
        j["width"] = tm.width;
        j["height"] = tm.height;
        j["tileWidth"] = tm.tileWidth;
        j["tileHeight"] = tm.tileHeight;

        j["tileset"] = json{
            {"texturePath", tm.tileset.texturePath},
            {"tileWidth", tm.tileset.tileWidth},
            {"tileHeight", tm.tileset.tileHeight},
            {"columns", tm.tileset.columns},
            {"margin", tm.tileset.margin},
            {"spacing", tm.tileset.spacing}
        };

        json layers = json::array();
        for (const auto& L : tm.layers)
        {
            json jl;
            jl["name"] = L.name;
            jl["layer"] = L.layer;
            jl["visible"] = L.visible;
            jl["tint"] = ColorToJson(L.tint);

            // tiles can be big; still ok for now
            jl["tiles"] = L.tiles;
            layers.push_back(std::move(jl));
        }

        j["layers"] = std::move(layers);
        e["Tilemap"] = std::move(j);
    }

    static void SaveTilemapCollider(json& e, const TilemapColliderComponent& c)
    {
        e["TilemapCollider"] = json{
            {"collisionLayerIndex", c.collisionLayerIndex},
            {"friction", c.friction},
            {"restitution", c.restitution},
            {"isSensor", c.isSensor},
            {"slopeUpRightTiles", c.slopeUpRightTiles},
            {"slopeUpLeftTiles", c.slopeUpLeftTiles},
            {"slopeFriction", c.slopeFriction}
        };
    }

    static void SaveRigidBody(json& e, const RigidBody2DComponent& c)
    {
        e["RigidBody2D"] = json{
            {"type", BodyTypeToString(c.type)},
            {"enabled", c.enabled},
            {"fixedRotation", c.fixedRotation},
            {"enableSleep", c.enableSleep},
            {"isBullet", c.isBullet},
            {"gravityScale", c.gravityScale},
            {"linearDamping", c.linearDamping},
            {"angularDamping", c.angularDamping}
        };
    }

    static void SaveBoxCollider(json& e, const BoxCollider2DComponent& c)
    {
        e["BoxCollider2D"] = json{
            {"size", Vec2ToJson(c.size)},
            {"offset", Vec2ToJson(c.offset)},
            {"enabled", c.enabled},
            {"density", c.density},
            {"friction", c.friction},
            {"restitution", c.restitution},
            {"isSensor", c.isSensor},
            {"categoryBits", c.categoryBits},
            {"maskBits", c.maskBits}
        };
    }

    static void SavePlatformer(json& e, const PlatformerControllerComponent& c)
    {
        e["PlatformerController"] = json{
            {"moveSpeedPx", c.moveSpeedPx},
            {"accelPx", c.accelPx},
            {"decelPx", c.decelPx},
            {"jumpSpeedPx", c.jumpSpeedPx},
            {"coyoteTime", c.coyoteTime},
            {"jumpBufferTime", c.jumpBufferTime},
            {"groundCheckDistancePx", c.groundCheckDistancePx},
            {"groundRayInsetPx", c.groundRayInsetPx},
            {"left", (int)c.left},
            {"right", (int)c.right},
            {"jump", (int)c.jump},
            {"dash", (int)c.dash},
            {"dashSpeedPx", c.dashSpeedPx},
            {"dashTime", c.dashTime},
            {"dashCooldown", c.dashCooldown},
            {"maxGroundSlopeDeg", c.maxGroundSlopeDeg},
            {"maxJumpSlopeDeg", c.maxJumpSlopeDeg}
        };
    }

    static void SavePersistentFlag(json& e, const PersistentFlagComponent& c)
    {
        e["PersistentFlag"] = json{ {"flag", c.flag} };
    }

    static void SaveGrantProgression(json& e, const GrantProgressionComponent& c)
    {
        e["GrantProgression"] = json{
            {"setFlag", c.setFlag},
            {"unlockAbility", c.unlockAbility},
            {"ability", std::string(AbilityName(c.ability))},
            {"radiusPx", c.radiusPx}
        };
    }

    static void SaveGate(json& e, const GateComponent& c)
    {
        e["Gate"] = json{
            {"requireAllAbilities", AbilityListToJson(c.requireAllAbilities)},
            {"requireAnyAbilities", AbilityListToJson(c.requireAnyAbilities)},
            {"requireAllFlags", c.requireAllFlags},
            {"requireAnyFlags", c.requireAnyFlags},
            {"invert", c.invert},
            {"openWhenSatisfied", c.openWhenSatisfied},
            {"openBehavior", GateBehaviorToString(c.openBehavior)},
            {"overrideTint", c.overrideTint},
            {"closedTint", ColorToJson(c.closedTint)},
            {"openTint", ColorToJson(c.openTint)},
            {"hideWhenOpen", c.hideWhenOpen}
        };
    }

    static void LoadPlayerSpawn(entt::registry& reg, entt::entity e, const json& j)
    {
        auto& c = reg.emplace_or_replace<PlayerSpawnComponent>(e);
        c.name = j.value("name", c.name);
    }

    static void LoadDoor(entt::registry& reg, entt::entity e, const json& j)
    {
        auto& c = reg.emplace_or_replace<DoorComponent>(e);
        c.targetScene = j.value("targetScene", c.targetScene);
        c.targetSpawn = j.value("targetSpawn", c.targetSpawn);
        c.triggerSize = JsonToVec2(j.value("triggerSize", json{}), c.triggerSize);
        c.requireInteract = j.value("requireInteract", c.requireInteract);
        c.interactKey = (SDL_Scancode)j.value("interactKey", (int)c.interactKey);
        c.autoTrigger = j.value("autoTrigger", c.autoTrigger);

        // runtime reset
        c.cooldownTimer = 0.0f;
    }

    // ---- component load ----
    static void LoadTransform(entt::registry& reg, entt::entity e, const json& j)
    {
        auto& c = reg.get<TransformComponent>(e);
        c.position = JsonToVec2(j.value("position", json{}), c.position);
        c.rotationDeg = j.value("rotationDeg", c.rotationDeg);
        c.scale = JsonToVec2(j.value("scale", json{}), c.scale);
    }

    static void LoadSprite(entt::registry& reg, entt::entity e, const json& j)
    {
        auto& c = reg.emplace_or_replace<SpriteRendererComponent>(e);
        c.texturePath = j.value("texturePath", c.texturePath);
        c.size = JsonToVec2(j.value("size", json{}), c.size);
        c.tint = JsonToColor(j.value("tint", json{}), c.tint);
        c.layer = j.value("layer", c.layer);
        c.useSourceRect = j.value("useSourceRect", c.useSourceRect);
        c.sourceRect = JsonToRect(j.value("sourceRect", json{}), c.sourceRect);
        c.flip = (SDL_RendererFlip)j.value("flip", (int)c.flip);
    }

    static void LoadTilemap(entt::registry& reg, entt::entity e, const json& j)
    {
        auto& tm = reg.emplace_or_replace<TilemapComponent>(e);
        tm.width = j.value("width", tm.width);
        tm.height = j.value("height", tm.height);
        tm.tileWidth = j.value("tileWidth", tm.tileWidth);
        tm.tileHeight = j.value("tileHeight", tm.tileHeight);

        if (j.contains("tileset") && j["tileset"].is_object())
        {
            const auto& ts = j["tileset"];
            tm.tileset.texturePath = ts.value("texturePath", tm.tileset.texturePath);
            tm.tileset.tileWidth = ts.value("tileWidth", tm.tileset.tileWidth);
            tm.tileset.tileHeight = ts.value("tileHeight", tm.tileset.tileHeight);
            tm.tileset.columns = ts.value("columns", tm.tileset.columns);
            tm.tileset.margin = ts.value("margin", tm.tileset.margin);
            tm.tileset.spacing = ts.value("spacing", tm.tileset.spacing);
        }

        tm.layers.clear();
        if (j.contains("layers") && j["layers"].is_array())
        {
            for (auto& it : j["layers"])
            {
                TileLayer L;
                L.name = it.value("name", L.name);
                L.layer = it.value("layer", L.layer);
                L.visible = it.value("visible", L.visible);
                L.tint = JsonToColor(it.value("tint", json{}), L.tint);

                if (it.contains("tiles") && it["tiles"].is_array())
                    L.tiles = it["tiles"].get<std::vector<int>>();

                tm.layers.push_back(std::move(L));
            }
        }
    }

    static void LoadTilemapCollider(entt::registry& reg, entt::entity e, const json& j)
    {
        auto& c = reg.emplace_or_replace<TilemapColliderComponent>(e);
        c.collisionLayerIndex = j.value("collisionLayerIndex", c.collisionLayerIndex);
        c.friction = j.value("friction", c.friction);
        c.restitution = j.value("restitution", c.restitution);
        c.isSensor = j.value("isSensor", c.isSensor);

        if (j.contains("slopeUpRightTiles")) c.slopeUpRightTiles = j["slopeUpRightTiles"].get<std::vector<int>>();
        if (j.contains("slopeUpLeftTiles")) c.slopeUpLeftTiles = j["slopeUpLeftTiles"].get<std::vector<int>>();
        c.slopeFriction = j.value("slopeFriction", c.slopeFriction);

        c.runtimeBodies.clear(); // runtime only
    }

    static void LoadRigidBody(entt::registry& reg, entt::entity e, const json& j)
    {
        auto& c = reg.emplace_or_replace<RigidBody2DComponent>(e);
        c.type = BodyTypeFromString(j.value("type", std::string(BodyTypeToString(c.type))), c.type);
        c.enabled = j.value("enabled", c.enabled);
        c.fixedRotation = j.value("fixedRotation", c.fixedRotation);
        c.enableSleep = j.value("enableSleep", c.enableSleep);
        c.isBullet = j.value("isBullet", c.isBullet);
        c.gravityScale = j.value("gravityScale", c.gravityScale);
        c.linearDamping = j.value("linearDamping", c.linearDamping);
        c.angularDamping = j.value("angularDamping", c.angularDamping);

        c.bodyId = b2_nullBodyId; // runtime
    }

    static void LoadBoxCollider(entt::registry& reg, entt::entity e, const json& j)
    {
        auto& c = reg.emplace_or_replace<BoxCollider2DComponent>(e);
        c.size = JsonToVec2(j.value("size", json{}), c.size);
        c.offset = JsonToVec2(j.value("offset", json{}), c.offset);
        c.enabled = j.value("enabled", c.enabled);
        c.density = j.value("density", c.density);
        c.friction = j.value("friction", c.friction);
        c.restitution = j.value("restitution", c.restitution);
        c.isSensor = j.value("isSensor", c.isSensor);
        c.categoryBits = j.value("categoryBits", c.categoryBits);
        c.maskBits = j.value("maskBits", c.maskBits);

        c.shapeId = b2_nullShapeId; // runtime
    }

    static void LoadPlatformer(entt::registry& reg, entt::entity e, const json& j)
    {
        auto& c = reg.emplace_or_replace<PlatformerControllerComponent>(e);
        c.moveSpeedPx = j.value("moveSpeedPx", c.moveSpeedPx);
        c.accelPx = j.value("accelPx", c.accelPx);
        c.decelPx = j.value("decelPx", c.decelPx);

        c.jumpSpeedPx = j.value("jumpSpeedPx", c.jumpSpeedPx);
        c.coyoteTime = j.value("coyoteTime", c.coyoteTime);
        c.jumpBufferTime = j.value("jumpBufferTime", c.jumpBufferTime);
        c.groundCheckDistancePx = j.value("groundCheckDistancePx", c.groundCheckDistancePx);
        c.groundRayInsetPx = j.value("groundRayInsetPx", c.groundRayInsetPx);

        c.left = (SDL_Scancode)j.value("left", (int)c.left);
        c.right = (SDL_Scancode)j.value("right", (int)c.right);
        c.jump = (SDL_Scancode)j.value("jump", (int)c.jump);
        c.dash = (SDL_Scancode)j.value("dash", (int)c.dash);

        c.dashSpeedPx = j.value("dashSpeedPx", c.dashSpeedPx);
        c.dashTime = j.value("dashTime", c.dashTime);
        c.dashCooldown = j.value("dashCooldown", c.dashCooldown);

        c.maxGroundSlopeDeg = j.value("maxGroundSlopeDeg", c.maxGroundSlopeDeg);
        c.maxJumpSlopeDeg = j.value("maxJumpSlopeDeg", c.maxJumpSlopeDeg);

        // runtime reset
        c.facing = 1;
        c.isDashing = false;
        c.dashTimer = 0.0f;
        c.dashCooldownTimer = 0.0f;
        c.dashDir = 1;
        c.jumpableGround = false;
        c.grounded = false;
        c.coyoteTimer = 0.0f;
        c.jumpBufferTimer = 0.0f;
    }

    static void LoadPersistentFlag(entt::registry& reg, entt::entity e, const json& j)
    {
        auto& c = reg.emplace_or_replace<PersistentFlagComponent>(e);
        c.flag = j.value("flag", c.flag);
    }

    static void LoadGrantProgression(entt::registry& reg, entt::entity e, const json& j)
    {
        auto& c = reg.emplace_or_replace<GrantProgressionComponent>(e);
        c.setFlag = j.value("setFlag", c.setFlag);
        c.unlockAbility = j.value("unlockAbility", c.unlockAbility);
        c.radiusPx = j.value("radiusPx", c.radiusPx);

        if (j.contains("ability") && j["ability"].is_string())
        {
            AbilityId a{};
            if (AbilityFromName(j["ability"].get<std::string>(), a))
                c.ability = a;
        }
    }

    static void LoadGate(entt::registry& reg, entt::entity e, const json& j)
    {
        auto& c = reg.emplace_or_replace<GateComponent>(e);

        c.requireAllAbilities = AbilityListFromJson(j.value("requireAllAbilities", json::array()));
        c.requireAnyAbilities = AbilityListFromJson(j.value("requireAnyAbilities", json::array()));

        if (j.contains("requireAllFlags")) c.requireAllFlags = j["requireAllFlags"].get<std::vector<std::string>>();
        if (j.contains("requireAnyFlags")) c.requireAnyFlags = j["requireAnyFlags"].get<std::vector<std::string>>();

        c.invert = j.value("invert", c.invert);
        c.openWhenSatisfied = j.value("openWhenSatisfied", c.openWhenSatisfied);
        c.openBehavior = GateBehaviorFromString(j.value("openBehavior", std::string(GateBehaviorToString(c.openBehavior))), c.openBehavior);

        c.overrideTint = j.value("overrideTint", c.overrideTint);
        c.closedTint = JsonToColor(j.value("closedTint", json{}), c.closedTint);
        c.openTint = JsonToColor(j.value("openTint", json{}), c.openTint);
        c.hideWhenOpen = j.value("hideWhenOpen", c.hideWhenOpen);

        // runtime reset
        c.initialized = false;
        c.isOpen = false;
    }

    // ---- main API ----
    bool SceneSerializer::SaveToFile(const Scene& scene, const std::string& path)
    {
        const auto& reg = const_cast<Scene&>(scene).Registry(); // entt view needs non-const registry

        json root;
        root["sceneVersion"] = 1;
        root["entities"] = json::array();

        auto view = reg.view<IdComponent, TagComponent>();
        for (auto ent : view)
        {
            const auto& idc = view.get<IdComponent>(ent);
            const auto& tag = view.get<TagComponent>(ent);

            json e;
            e["id"] = idc.id;
            e["tag"] = tag.tag;

            // Transform always exists in your CreateEntity, but be safe:
            if (reg.any_of<TransformComponent>(ent))
                SaveTransform(e, reg.get<TransformComponent>(ent));

            if (reg.any_of<SpriteRendererComponent>(ent))
                SaveSprite(e, reg.get<SpriteRendererComponent>(ent));

            if (reg.any_of<TilemapComponent>(ent))
                SaveTilemap(e, reg.get<TilemapComponent>(ent));

            if (reg.any_of<TilemapColliderComponent>(ent))
                SaveTilemapCollider(e, reg.get<TilemapColliderComponent>(ent));

            if (reg.any_of<RigidBody2DComponent>(ent))
                SaveRigidBody(e, reg.get<RigidBody2DComponent>(ent));

            if (reg.any_of<BoxCollider2DComponent>(ent))
                SaveBoxCollider(e, reg.get<BoxCollider2DComponent>(ent));

            if (reg.any_of<PlatformerControllerComponent>(ent))
                SavePlatformer(e, reg.get<PlatformerControllerComponent>(ent));

            if (reg.any_of<PersistentFlagComponent>(ent))
                SavePersistentFlag(e, reg.get<PersistentFlagComponent>(ent));

            if (reg.any_of<GrantProgressionComponent>(ent))
                SaveGrantProgression(e, reg.get<GrantProgressionComponent>(ent));

            if (reg.any_of<GateComponent>(ent))
                SaveGate(e, reg.get<GateComponent>(ent));

            if (reg.any_of<PlayerSpawnComponent>(ent))
                SavePlayerSpawn(e, reg.get<PlayerSpawnComponent>(ent));

            if (reg.any_of<DoorComponent>(ent))
                SaveDoor(e, reg.get<DoorComponent>(ent));

            root["entities"].push_back(std::move(e));
        }

        std::ofstream out(path, std::ios::binary);
        if (!out) return false;
        out << root.dump(2);
        return true;
    }

    bool SceneSerializer::LoadFromFile(Scene& scene, const std::string& path)
    {
        std::ifstream in(path, std::ios::binary);
        if (!in) return false;

        json root;
        in >> root;

        auto& reg = scene.Registry();
        reg.clear();

        if (!root.contains("entities") || !root["entities"].is_array())
            return false;

        for (auto& je : root["entities"])
        {
            const uint64_t id = je.value("id", 0ull);
            const std::string tag = je.value("tag", std::string("Entity"));

            Entity ent = scene.CreateEntityWithId(id, tag);
            const entt::entity h = ent.Handle();

            if (je.contains("Transform")) LoadTransform(reg, h, je["Transform"]);
            if (je.contains("SpriteRenderer")) LoadSprite(reg, h, je["SpriteRenderer"]);
            if (je.contains("Tilemap")) LoadTilemap(reg, h, je["Tilemap"]);
            if (je.contains("TilemapCollider")) LoadTilemapCollider(reg, h, je["TilemapCollider"]);
            if (je.contains("RigidBody2D")) LoadRigidBody(reg, h, je["RigidBody2D"]);
            if (je.contains("BoxCollider2D")) LoadBoxCollider(reg, h, je["BoxCollider2D"]);
            if (je.contains("PlatformerController")) LoadPlatformer(reg, h, je["PlatformerController"]);
            if (je.contains("PersistentFlag")) LoadPersistentFlag(reg, h, je["PersistentFlag"]);
            if (je.contains("GrantProgression")) LoadGrantProgression(reg, h, je["GrantProgression"]);
            if (je.contains("Gate")) LoadGate(reg, h, je["Gate"]);
            if (je.contains("PlayerSpawn")) LoadPlayerSpawn(reg, h, je["PlayerSpawn"]);
            if (je.contains("Door")) LoadDoor(reg, h, je["Door"]);
        }

        return true;
    }
}