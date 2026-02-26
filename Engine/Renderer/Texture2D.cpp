#include "pch.h"
#include "Renderer/Texture2D.h"
#include "Platform/SdlImage.h"

#include <spdlog/spdlog.h>

namespace my2d
{
    Texture2D::~Texture2D()
    {
        if (m_texture)
        {
            SDL_DestroyTexture(m_texture);
            m_texture = nullptr;
        }
    }

    Texture2D::Texture2D(Texture2D&& other) noexcept
    {
        *this = std::move(other);
    }

    Texture2D& Texture2D::operator=(Texture2D&& other) noexcept
    {
        if (this == &other) return *this;

        if (m_texture) SDL_DestroyTexture(m_texture);

        m_texture = other.m_texture;
        m_width = other.m_width;
        m_height = other.m_height;
        m_path = std::move(other.m_path);

        other.m_texture = nullptr;
        other.m_width = 0;
        other.m_height = 0;

        return *this;
    }

    bool Texture2D::LoadFromFile(SDL_Renderer* renderer, const std::string& path)
    {
        if (!renderer)
        {
            spdlog::error("Texture2D::LoadFromFile: renderer is null");
            return false;
        }

        SDL_Surface* surface = IMG_Load(path.c_str());
        if (!surface)
        {
            spdlog::error("IMG_Load failed for '{}': {}", path, IMG_GetError());
            return false;
        }

        SDL_Texture* tex = SDL_CreateTextureFromSurface(renderer, surface);
        m_width = surface->w;
        m_height = surface->h;

        SDL_FreeSurface(surface);

        if (!tex)
        {
            spdlog::error("SDL_CreateTextureFromSurface failed for '{}': {}", path, SDL_GetError());
            return false;
        }

        // Common default for sprites with alpha
        SDL_SetTextureBlendMode(tex, SDL_BLENDMODE_BLEND);

        if (m_texture) SDL_DestroyTexture(m_texture);
        m_texture = tex;
        m_path = path;

        return true;
    }
}