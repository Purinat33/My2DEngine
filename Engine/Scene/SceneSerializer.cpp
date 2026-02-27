#include "pch.h"
#include "Scene/SceneSerializer.h"

#include "Scene/Scene.h"
#include "Scene/Components.h"

#include <nlohmann/json.hpp>
#include <fstream>
#include <filesystem>

#include <spdlog/spdlog.h>

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

    static std::string JoinRelativeToFile2(const std::string& filePath, const std::string& rel)
    {
        namespace fs = std::filesystem;
        fs::path base = fs::path(filePath).parent_path();
        fs::path p = fs::path(rel);
        if (p.is_absolute() || p.has_root_name())
            return p.lexically_normal().string();
        return (base / p).lexically_normal().string();
    }

    static bool LoadJsonFile2(const std::string& path, nlohmann::json& out)
    {
        std::ifstream in(path, std::ios::binary);
        if (!in) return false;
        in >> out;
        return true;
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


    // ---- Facing ----
    static void SaveFacing(json& e, const FacingComponent& c)
    {
        e["Facing"] = json{ {"facing", c.facing} };
    }

    static void LoadFacing(entt::registry& reg, entt::entity e, const json& j)
    {
        auto& c = reg.emplace_or_replace<FacingComponent>(e);
        c.facing = j.value("facing", c.facing);
    }

    // ---- Team ----
    static const char* TeamToString(Team t)
    {
        switch (t)
        {
        case Team::Player: return "Player";
        case Team::Enemy: return "Enemy";
        default: return "Neutral";
        }
    }

    static Team TeamFromString(const std::string& s)
    {
        if (s == "Player") return Team::Player;
        if (s == "Enemy") return Team::Enemy;
        return Team::Neutral;
    }

    static void SaveTeam(json& e, const TeamComponent& c)
    {
        e["Team"] = json{ {"team", TeamToString(c.team)} };
    }

    static void LoadTeam(entt::registry& reg, entt::entity e, const json& j)
    {
        auto& c = reg.emplace_or_replace<TeamComponent>(e);
        c.team = TeamFromString(j.value("team", std::string(TeamToString(c.team))));
    }

    // ---- Health ----
    static void SaveHealth(json& e, const HealthComponent& c)
    {
        e["Health"] = json{ {"maxHp", c.maxHp}, {"hp", c.hp} };
    }

    static void LoadHealth(entt::registry& reg, entt::entity e, const json& j)
    {
        auto& c = reg.emplace_or_replace<HealthComponent>(e);
        c.maxHp = j.value("maxHp", c.maxHp);
        c.hp = j.value("hp", c.hp);
    }

    // ---- Hurtbox ----
    static void SaveHurtbox(json& e, const HurtboxComponent& c)
    {
        e["Hurtbox"] = json{ {"enabled", c.enabled} };
    }

    static void LoadHurtbox(entt::registry& reg, entt::entity e, const json& j)
    {
        auto& c = reg.emplace_or_replace<HurtboxComponent>(e);
        c.enabled = j.value("enabled", c.enabled);
    }

    // ---- MeleeAttack (CONFIG ONLY; do NOT serialize runtime timers/shape ids) ----
    static void SaveMeleeAttack(json& e, const MeleeAttackComponent& c)
    {
        e["MeleeAttack"] = json{
            {"attackKey", (int)c.attackKey},
            {"useInput", c.useInput},

            {"damage", c.damage},
            {"activeTime", c.activeTime},
            {"cooldown", c.cooldown},

            {"hitboxSizePx", Vec2ToJson(c.hitboxSizePx)},
            {"hitboxOffsetPx", Vec2ToJson(c.hitboxOffsetPx)},

            {"allowAimUp", c.allowAimUp},
            {"allowAimDown", c.allowAimDown},
            {"allowDownAttackOnGround", c.allowDownAttackOnGround},
            {"aimUpKey", (int)c.aimUpKey},
            {"aimDownKey", (int)c.aimDownKey},

            {"hitboxSizeUpPx", Vec2ToJson(c.hitboxSizeUpPx)},
            {"hitboxOffsetUpPx", Vec2ToJson(c.hitboxOffsetUpPx)},
            {"hitboxSizeDownPx", Vec2ToJson(c.hitboxSizeDownPx)},
            {"hitboxOffsetDownPx", Vec2ToJson(c.hitboxOffsetDownPx)},

            {"knockbackSpeedPx", c.knockbackSpeedPx},
            {"knockbackUpPx", c.knockbackUpPx},
            {"victimInvuln", c.victimInvuln},
            {"targetMaskBits", c.targetMaskBits},

            {"pogoOnDownHit", c.pogoOnDownHit},
            {"pogoReboundSpeedPx", c.pogoReboundSpeedPx}
        };
    }

    static void LoadMeleeAttack(entt::registry& reg, entt::entity e, const json& j)
    {
        auto& c = reg.emplace_or_replace<MeleeAttackComponent>(e);

        c.attackKey = (SDL_Scancode)j.value("attackKey", (int)c.attackKey);
        c.useInput = j.value("useInput", c.useInput);

        c.damage = j.value("damage", c.damage);
        c.activeTime = (float)j.value("activeTime", (double)c.activeTime);
        c.cooldown = (float)j.value("cooldown", (double)c.cooldown);

        c.hitboxSizePx = JsonToVec2(j.value("hitboxSizePx", json{}), c.hitboxSizePx);
        c.hitboxOffsetPx = JsonToVec2(j.value("hitboxOffsetPx", json{}), c.hitboxOffsetPx);

        c.allowAimUp = j.value("allowAimUp", c.allowAimUp);
        c.allowAimDown = j.value("allowAimDown", c.allowAimDown);
        c.allowDownAttackOnGround = j.value("allowDownAttackOnGround", c.allowDownAttackOnGround);
        c.aimUpKey = (SDL_Scancode)j.value("aimUpKey", (int)c.aimUpKey);
        c.aimDownKey = (SDL_Scancode)j.value("aimDownKey", (int)c.aimDownKey);

        c.hitboxSizeUpPx = JsonToVec2(j.value("hitboxSizeUpPx", json{}), c.hitboxSizeUpPx);
        c.hitboxOffsetUpPx = JsonToVec2(j.value("hitboxOffsetUpPx", json{}), c.hitboxOffsetUpPx);
        c.hitboxSizeDownPx = JsonToVec2(j.value("hitboxSizeDownPx", json{}), c.hitboxSizeDownPx);
        c.hitboxOffsetDownPx = JsonToVec2(j.value("hitboxOffsetDownPx", json{}), c.hitboxOffsetDownPx);

        c.knockbackSpeedPx = (float)j.value("knockbackSpeedPx", (double)c.knockbackSpeedPx);
        c.knockbackUpPx = (float)j.value("knockbackUpPx", (double)c.knockbackUpPx);
        c.victimInvuln = (float)j.value("victimInvuln", (double)c.victimInvuln);
        c.targetMaskBits = (uint64_t)j.value("targetMaskBits", (uint64_t)c.targetMaskBits);

        c.pogoOnDownHit = j.value("pogoOnDownHit", c.pogoOnDownHit);
        c.pogoReboundSpeedPx = (float)j.value("pogoReboundSpeedPx", (double)c.pogoReboundSpeedPx);

        // runtime reset
        c.activeTimer = 0.0f;
        c.cooldownTimer = 0.0f;
        c.hitboxShapeId = b2_nullShapeId;
        c.runtimeUserData = nullptr;
        c.attackRequested = false;
    }

    // ---- EnemyAI ----
    static void SaveEnemyAI(json& e, const EnemyAIComponent& c)
    {
        e["EnemyAI"] = json{
            {"patrolSpeedPx", c.patrolSpeedPx},
            {"chaseSpeedPx", c.chaseSpeedPx},
            {"aggroRangePx", c.aggroRangePx},
            {"deaggroRangePx", c.deaggroRangePx},
            {"attackRangePx", c.attackRangePx},
            {"ledgeCheckDownPx", c.ledgeCheckDownPx},
            {"wallCheckForwardPx", c.wallCheckForwardPx},
            {"canPatrol", c.canPatrol},
            {"canChase", c.canChase}
        };
    }

    static void LoadEnemyAI(entt::registry& reg, entt::entity e, const json& j)
    {
        auto& c = reg.emplace_or_replace<EnemyAIComponent>(e);
        c.patrolSpeedPx = (float)j.value("patrolSpeedPx", (double)c.patrolSpeedPx);
        c.chaseSpeedPx = (float)j.value("chaseSpeedPx", (double)c.chaseSpeedPx);
        c.aggroRangePx = (float)j.value("aggroRangePx", (double)c.aggroRangePx);
        c.deaggroRangePx = (float)j.value("deaggroRangePx", (double)c.deaggroRangePx);
        c.attackRangePx = (float)j.value("attackRangePx", (double)c.attackRangePx);
        c.ledgeCheckDownPx = (float)j.value("ledgeCheckDownPx", (double)c.ledgeCheckDownPx);
        c.wallCheckForwardPx = (float)j.value("wallCheckForwardPx", (double)c.wallCheckForwardPx);
        c.canPatrol = j.value("canPatrol", c.canPatrol);
        c.canChase = j.value("canChase", c.canChase);

        // runtime reset
        c.aggro = false;
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
            {"flip", (int)c.flip},
            {"atlasPath", c.atlasPath},
            {"regionName", c.regionName},
            {"pivot", Vec2ToJson(c.pivot)},
            {"offset", Vec2ToJson(c.offset)},
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
        c.atlasPath = j.value("atlasPath", c.atlasPath);
        c.regionName = j.value("regionName", c.regionName);
        c.pivot = JsonToVec2(j.value("pivot", json{}), c.pivot);
        c.offset = JsonToVec2(j.value("offset", json{}), c.offset);
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

    static void SaveAnimator(json& e, const AnimatorComponent& c)
    {
        e["Animator"] = json{
            {"animSetPath", c.animSetPath},
            {"clip", c.clip},
            {"playing", c.playing},
            {"speed", c.speed}
        };
    }

    static void LoadAnimator(entt::registry& reg, entt::entity e, const json& j)
    {
        auto& c = reg.emplace_or_replace<AnimatorComponent>(e);
        c.animSetPath = j.value("animSetPath", c.animSetPath);
        c.clip = j.value("clip", c.clip);
        c.playing = j.value("playing", c.playing);
        c.speed = (float)j.value("speed", (double)c.speed);

        // runtime reset
        c.time = 0.0f;
        c.frameIndex = 0;
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
            if (reg.any_of<PrefabComponent>(ent))
                e["prefab"] = reg.get<PrefabComponent>(ent).prefabPath;

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

            if (reg.any_of<AnimatorComponent>(ent))
                SaveAnimator(e, reg.get<AnimatorComponent>(ent));

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

            if (reg.any_of<FacingComponent>(ent)) SaveFacing(e, reg.get<FacingComponent>(ent));
            if (reg.any_of<TeamComponent>(ent)) SaveTeam(e, reg.get<TeamComponent>(ent));
            if (reg.any_of<HealthComponent>(ent)) SaveHealth(e, reg.get<HealthComponent>(ent));
            if (reg.any_of<HurtboxComponent>(ent)) SaveHurtbox(e, reg.get<HurtboxComponent>(ent));
            if (reg.any_of<MeleeAttackComponent>(ent)) SaveMeleeAttack(e, reg.get<MeleeAttackComponent>(ent));
            if (reg.any_of<EnemyAIComponent>(ent)) SaveEnemyAI(e, reg.get<EnemyAIComponent>(ent));

            root["entities"].push_back(std::move(e));
        }
        namespace fs = std::filesystem;
        fs::path p(path);
        p = p.lexically_normal().make_preferred();
        fs::create_directories(p.parent_path());

        std::ofstream out(p, std::ios::binary);
        if (!out) return false;
        out << root.dump(2);
        return true;
    }

    bool SceneSerializer::LoadFromFile(Scene& scene, const std::string& path)
    {
        namespace fs = std::filesystem;

        fs::path p(path);
        p = p.lexically_normal().make_preferred();

        if (!fs::exists(p))
        {
            spdlog::error("SceneSerializer: file does not exist: {}", p.string());
            spdlog::error("CWD: {}", fs::current_path().string());
            return false;
        }

        std::ifstream in(p, std::ios::binary);
        if (!in)
        {
            spdlog::error("SceneSerializer: cannot open: {}", p.string());
            spdlog::error("CWD: {}", fs::current_path().string());
            return false;
        }

        json root;
        try
        {
            in >> root;
        }
        catch (const std::exception& ex)
        {
            spdlog::error("SceneSerializer: JSON parse failed for '{}': {}", p.string(), ex.what());
            return false;
        }

        auto& reg = scene.Registry();
        reg.clear();

        if (!root.contains("entities") || !root["entities"].is_array())
        {
            spdlog::error("SceneSerializer: '{}' missing 'entities' array", p.string());
            return false;
        }

        for (auto& je : root["entities"])
        {
            const uint64_t id = je.value("id", 0ull);

            // Choose tag: scene tag overrides prefab tag
            std::string tag = je.value("tag", std::string("Entity"));

            // Optional prefab
            nlohmann::json baseEntity;
            bool hasPrefab = false;

            if (je.contains("prefab") && je["prefab"].is_string())
            {
                const std::string prefabRel = je["prefab"].get<std::string>();
                const std::string prefabFull = JoinRelativeToFile2(path, prefabRel);

                nlohmann::json prefRoot;
                if (!LoadJsonFile2(prefabFull, prefRoot))
                {
                    spdlog::error("SceneSerializer: failed to load prefab '{}'", prefabFull);
                }
                else
                {
                    // prefab file can be either:
                    // A) {"prefabVersion":1, "entity": { ...entity json... }}
                    // B) { ...entity json... } directly
                    if (prefRoot.contains("entity") && prefRoot["entity"].is_object())
                        baseEntity = prefRoot["entity"];
                    else
                        baseEntity = prefRoot;

                    // If scene didn't specify tag, allow prefab tag
                    if (!je.contains("tag") && baseEntity.contains("tag") && baseEntity["tag"].is_string())
                        tag = baseEntity["tag"].get<std::string>();

                    hasPrefab = true;
                }
            }

            // Create entity with stable id
            Entity ent = scene.CreateEntityWithId(id, tag);
            const entt::entity h = ent.Handle();

            // Store prefab link (if any)
            if (hasPrefab && je.contains("prefab") && je["prefab"].is_string())
            {
                auto& pc = reg.emplace_or_replace<PrefabComponent>(h);
                pc.prefabPath = je["prefab"].get<std::string>();
            }

            // 1) Apply prefab components first
            if (hasPrefab)
            {
                if (baseEntity.contains("Transform")) LoadTransform(reg, h, baseEntity["Transform"]);
                if (baseEntity.contains("SpriteRenderer")) LoadSprite(reg, h, baseEntity["SpriteRenderer"]);
                if (baseEntity.contains("Animator")) LoadAnimator(reg, h, baseEntity["Animator"]);
                if (baseEntity.contains("Tilemap")) LoadTilemap(reg, h, baseEntity["Tilemap"]);
                if (baseEntity.contains("TilemapCollider")) LoadTilemapCollider(reg, h, baseEntity["TilemapCollider"]);
                if (baseEntity.contains("RigidBody2D")) LoadRigidBody(reg, h, baseEntity["RigidBody2D"]);
                if (baseEntity.contains("BoxCollider2D")) LoadBoxCollider(reg, h, baseEntity["BoxCollider2D"]);
                if (baseEntity.contains("PlatformerController")) LoadPlatformer(reg, h, baseEntity["PlatformerController"]);
                if (baseEntity.contains("PersistentFlag")) LoadPersistentFlag(reg, h, baseEntity["PersistentFlag"]);
                if (baseEntity.contains("GrantProgression")) LoadGrantProgression(reg, h, baseEntity["GrantProgression"]);
                if (baseEntity.contains("Gate")) LoadGate(reg, h, baseEntity["Gate"]);
                // (Add your combat/AI loads here if you already implemented them)
                if (baseEntity.contains("Facing")) LoadFacing(reg, h, baseEntity["Facing"]);
                if (baseEntity.contains("Team")) LoadTeam(reg, h, baseEntity["Team"]);
                if (baseEntity.contains("Health")) LoadHealth(reg, h, baseEntity["Health"]);
                if (baseEntity.contains("Hurtbox")) LoadHurtbox(reg, h, baseEntity["Hurtbox"]);
                if (baseEntity.contains("MeleeAttack")) LoadMeleeAttack(reg, h, baseEntity["MeleeAttack"]);
                if (baseEntity.contains("EnemyAI")) LoadEnemyAI(reg, h, baseEntity["EnemyAI"]);
            }

            // 2) Apply scene entity overrides second
            if (je.contains("Transform")) LoadTransform(reg, h, je["Transform"]);
            if (je.contains("SpriteRenderer")) LoadSprite(reg, h, je["SpriteRenderer"]);
            if (je.contains("Animator")) LoadAnimator(reg, h, je["Animator"]);
            if (je.contains("Tilemap")) LoadTilemap(reg, h, je["Tilemap"]);
            if (je.contains("TilemapCollider")) LoadTilemapCollider(reg, h, je["TilemapCollider"]);
            if (je.contains("RigidBody2D")) LoadRigidBody(reg, h, je["RigidBody2D"]);
            if (je.contains("BoxCollider2D")) LoadBoxCollider(reg, h, je["BoxCollider2D"]);
            if (je.contains("PlatformerController")) LoadPlatformer(reg, h, je["PlatformerController"]);
            if (je.contains("PersistentFlag")) LoadPersistentFlag(reg, h, je["PersistentFlag"]);
            if (je.contains("GrantProgression")) LoadGrantProgression(reg, h, je["GrantProgression"]);
            if (je.contains("Gate")) LoadGate(reg, h, je["Gate"]);
            if (je.contains("Facing")) LoadFacing(reg, h, je["Facing"]);
            if (je.contains("Team")) LoadTeam(reg, h, je["Team"]);
            if (je.contains("Health")) LoadHealth(reg, h, je["Health"]);
            if (je.contains("Hurtbox")) LoadHurtbox(reg, h, je["Hurtbox"]);
            if (je.contains("MeleeAttack")) LoadMeleeAttack(reg, h, je["MeleeAttack"]);
            if (je.contains("EnemyAI")) LoadEnemyAI(reg, h, je["EnemyAI"]);
        }

        return true;
    }

}