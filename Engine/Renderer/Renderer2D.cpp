#include "pch.h"
#include "Renderer/Renderer2D.h"
#include "Renderer/Texture2D.h"

#include <spdlog/spdlog.h>

namespace my2d
{
    void Renderer2D::DrawTexture(
        const Texture2D& texture,
        const glm::vec2& worldPos,
        const glm::vec2& worldSize,
        const SDL_Rect* srcRect,
        float rotationDeg,
        SDL_RendererFlip flip,
        SDL_Color tint)
    {
        if (!m_renderer)
        {
            spdlog::error("Renderer2D::DrawTexture called before SetRenderer()");
            return;
        }

        SDL_Texture* native = texture.GetNative();
        if (!native)
            return;

        // Apply tint
        SDL_SetTextureColorMod(native, tint.r, tint.g, tint.b);
        SDL_SetTextureAlphaMod(native, tint.a);

        const glm::vec2 screen = m_camera.WorldToScreen(worldPos);

        SDL_FRect dst{};
        dst.x = screen.x;
        dst.y = screen.y;
        dst.w = worldSize.x * m_camera.Zoom();
        dst.h = worldSize.y * m_camera.Zoom();

        SDL_RenderCopyExF(m_renderer, native, srcRect, &dst, rotationDeg, nullptr, flip);
    }
}