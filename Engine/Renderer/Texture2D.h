#pragma once
#include <string>

#include "Platform/Sdl.h"

namespace my2d
{
    class Texture2D
    {
    public:
        Texture2D() = default;
        ~Texture2D();

        Texture2D(const Texture2D&) = delete;
        Texture2D& operator=(const Texture2D&) = delete;

        Texture2D(Texture2D&& other) noexcept;
        Texture2D& operator=(Texture2D&& other) noexcept;

        bool LoadFromFile(SDL_Renderer* renderer, const std::string& path);

        SDL_Texture* GetNative() const { return m_texture; }
        int Width() const { return m_width; }
        int Height() const { return m_height; }
        const std::string& Path() const { return m_path; }

    private:
        SDL_Texture* m_texture = nullptr;
        int m_width = 0;
        int m_height = 0;
        std::string m_path;
    };
}