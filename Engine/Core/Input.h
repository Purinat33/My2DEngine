#pragma once
#include "Platform/Sdl.h"
#include <array>

namespace my2d
{
    class Input
    {
    public:
        void BeginFrame()
        {
            m_prevKeys = m_currKeys;
            m_prevMouseButtons = m_currMouseButtons;
            m_mouseWheelX = 0;
            m_mouseWheelY = 0;
            m_mouseDeltaX = 0;
            m_mouseDeltaY = 0;
        }

        void OnEvent(const SDL_Event& e)
        {
            switch (e.type)
            {
            case SDL_KEYDOWN:
                if (!e.key.repeat) SetKey(e.key.keysym.scancode, true);
                break;
            case SDL_KEYUP:
                SetKey(e.key.keysym.scancode, false);
                break;
            case SDL_MOUSEBUTTONDOWN:
                SetMouseButton(e.button.button, true);
                break;
            case SDL_MOUSEBUTTONUP:
                SetMouseButton(e.button.button, false);
                break;
            case SDL_MOUSEMOTION:
                m_mouseX = e.motion.x;
                m_mouseY = e.motion.y;
                m_mouseDeltaX += e.motion.xrel;
                m_mouseDeltaY += e.motion.yrel;
                break;
            case SDL_MOUSEWHEEL:
                m_mouseWheelX += e.wheel.x;
                m_mouseWheelY += e.wheel.y;
                break;
            default:
                break;
            }
        }

        bool IsKeyDown(SDL_Scancode sc) const { return m_currKeys[sc] != 0; }
        bool WasKeyPressed(SDL_Scancode sc) const { return m_currKeys[sc] != 0 && m_prevKeys[sc] == 0; }
        bool WasKeyReleased(SDL_Scancode sc) const { return m_currKeys[sc] == 0 && m_prevKeys[sc] != 0; }

        bool IsMouseDown(uint8_t button) const { return (m_currMouseButtons & SDL_BUTTON(button)) != 0; }
        bool WasMousePressed(uint8_t button) const
        {
            const uint32_t mask = SDL_BUTTON(button);
            return (m_currMouseButtons & mask) != 0 && (m_prevMouseButtons & mask) == 0;
        }

        int MouseX() const { return m_mouseX; }
        int MouseY() const { return m_mouseY; }
        int MouseDeltaX() const { return m_mouseDeltaX; }
        int MouseDeltaY() const { return m_mouseDeltaY; }
        int MouseWheelX() const { return m_mouseWheelX; }
        int MouseWheelY() const { return m_mouseWheelY; }

    private:
        void SetKey(SDL_Scancode sc, bool down)
        {
            m_currKeys[sc] = down ? 1u : 0u;
        }

        void SetMouseButton(uint8_t button, bool down)
        {
            const uint32_t mask = SDL_BUTTON(button);
            if (down) m_currMouseButtons |= mask;
            else      m_currMouseButtons &= ~mask;
        }

    private:
        std::array<uint8_t, SDL_NUM_SCANCODES> m_currKeys{};
        std::array<uint8_t, SDL_NUM_SCANCODES> m_prevKeys{};

        uint32_t m_currMouseButtons = 0;
        uint32_t m_prevMouseButtons = 0;

        int m_mouseX = 0;
        int m_mouseY = 0;
        int m_mouseDeltaX = 0;
        int m_mouseDeltaY = 0;
        int m_mouseWheelX = 0;
        int m_mouseWheelY = 0;
    };
}