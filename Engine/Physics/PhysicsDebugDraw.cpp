#include "pch.h"
#include "Physics/PhysicsDebugDraw.h"

namespace my2d
{
    struct DebugCtx
    {
        SDL_Renderer* r = nullptr;
        float ppm = 100.0f;
        const Camera2D* cam = nullptr;
    };

    static SDL_FPoint ToScreen(const DebugCtx& c, b2Vec2 pMeters)
    {
        const glm::vec2 worldPx{ pMeters.x * c.ppm, pMeters.y * c.ppm };
        const glm::vec2 screen = c.cam->WorldToScreen(worldPx);
        return SDL_FPoint{ screen.x, screen.y };
    }

    static void HexToRGBA(b2HexColor color, Uint8& r, Uint8& g, Uint8& b, Uint8& a)
    {
        const uint32_t c = (uint32_t)color;
        r = (Uint8)((c >> 16) & 0xFF);
        g = (Uint8)((c >> 8) & 0xFF);
        b = (Uint8)(c & 0xFF);
        a = 180;
    }

    static void DrawPolygon(const b2Vec2* verts, int count, b2HexColor color, void* ctx)
    {
        auto& c = *(DebugCtx*)ctx;
        Uint8 r, g, b, a; HexToRGBA(color, r, g, b, a);
        SDL_SetRenderDrawBlendMode(c.r, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(c.r, r, g, b, a);

        for (int i = 0; i < count; ++i)
        {
            const b2Vec2 p0 = verts[i];
            const b2Vec2 p1 = verts[(i + 1) % count];
            const SDL_FPoint s0 = ToScreen(c, p0);
            const SDL_FPoint s1 = ToScreen(c, p1);
            SDL_RenderDrawLineF(c.r, s0.x, s0.y, s1.x, s1.y);
        }
    }

    static void DrawSolidPolygon(b2Transform xf, const b2Vec2* verts, int count, float /*radius*/, b2HexColor color, void* ctx)
    {
        // Transform local verts to world and draw outline
        std::vector<b2Vec2> w;
        w.reserve(count);
        for (int i = 0; i < count; ++i)
            w.push_back(b2TransformPoint(xf, verts[i]));

        DrawPolygon(w.data(), count, color, ctx);
    }

    static void DrawSegment(b2Vec2 p1, b2Vec2 p2, b2HexColor color, void* ctx)
    {
        auto& c = *(DebugCtx*)ctx;
        Uint8 r, g, b, a; HexToRGBA(color, r, g, b, a);
        SDL_SetRenderDrawBlendMode(c.r, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(c.r, r, g, b, a);

        const SDL_FPoint s0 = ToScreen(c, p1);
        const SDL_FPoint s1 = ToScreen(c, p2);
        SDL_RenderDrawLineF(c.r, s0.x, s0.y, s1.x, s1.y);
    }

    static void DrawCircle(b2Vec2 center, float radius, b2HexColor color, void* ctx)
    {
        auto& c = *(DebugCtx*)ctx;
        Uint8 r, g, b, a; HexToRGBA(color, r, g, b, a);
        SDL_SetRenderDrawBlendMode(c.r, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(c.r, r, g, b, a);

        const int segments = 20;
        b2Vec2 prev = { center.x + radius, center.y };
        for (int i = 1; i <= segments; ++i)
        {
            const float t = (float)i / (float)segments * 6.2831853f;
            b2Vec2 cur = { center.x + radius * cosf(t), center.y + radius * sinf(t) };
            DrawSegment(prev, cur, color, ctx);
            prev = cur;
        }
    }

    static void DrawSolidCircle(b2Transform xf, float radius, b2HexColor color, void* ctx)
    {
        // center is xf.p for circles in debug draw
        DrawCircle(xf.p, radius, color, ctx);
    }

    void PhysicsDebugDraw::Draw(SDL_Renderer* r, b2WorldId worldId, float ppm, const Camera2D& cam)
    {
        if (!r) return;
        if (!b2World_IsValid(worldId)) return;

        DebugCtx ctx;
        ctx.r = r;
        ctx.ppm = ppm;
        ctx.cam = &cam;

        b2DebugDraw dd = b2DefaultDebugDraw();
        dd.DrawPolygonFcn = &DrawPolygon;
        dd.DrawSolidPolygonFcn = &DrawSolidPolygon;
        dd.DrawSegmentFcn = &DrawSegment;
        dd.DrawCircleFcn = &DrawCircle;
        dd.DrawSolidCircleFcn = &DrawSolidCircle;

        dd.drawShapes = true;
        dd.drawJoints = false;
        dd.drawBounds = false;
        dd.drawContacts = false;

        dd.context = &ctx;

        b2World_Draw(worldId, &dd);
    }
}