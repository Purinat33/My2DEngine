#pragma once
#include "Platform/Sdl.h"
#include "Renderer/Camera2D.h"

#include <glm/vec2.hpp>

namespace my2d
{
    class Texture2D;

    class Renderer2D
    {
    public:
        void SetRenderer(SDL_Renderer* r) { m_renderer = r; }
        SDL_Renderer* GetRenderer() const { return m_renderer; }

        Camera2D& GetCamera() { return m_camera; }
        const Camera2D& GetCamera() const { return m_camera; }

        void SetViewport(int w, int h) { m_camera.SetViewport(w, h); }

        void DrawTexture(
            const Texture2D& texture,
            const glm::vec2& worldPos,
            const glm::vec2& worldSize,
            const SDL_Rect* srcRect = nullptr,
            float rotationDeg = 0.0f,
            SDL_RendererFlip flip = SDL_FLIP_NONE,
            SDL_Color tint = { 255, 255, 255, 255 }
        );

    private:
        SDL_Renderer* m_renderer = nullptr;
        Camera2D m_camera;
    };
}