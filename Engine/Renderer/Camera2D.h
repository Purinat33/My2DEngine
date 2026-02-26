#pragma once
#include <glm/vec2.hpp>

namespace my2d
{
    class Camera2D
    {
    public:
        void SetViewport(int w, int h) { m_viewW = w; m_viewH = h; }
        void SetPosition(const glm::vec2& p) { m_position = p; }
        void SetZoom(float z) { m_zoom = (z <= 0.0001f) ? 0.0001f : z; }

        const glm::vec2& Position() const { return m_position; }
        float Zoom() const { return m_zoom; }

        glm::vec2 WorldToScreen(const glm::vec2& world) const
        {
            const glm::vec2 half = { m_viewW * 0.5f, m_viewH * 0.5f };
            return (world - m_position) * m_zoom + half;
        }

    private:
        glm::vec2 m_position{ 0.0f, 0.0f }; // world position at screen center
        float m_zoom = 1.0f;

        int m_viewW = 1280;
        int m_viewH = 720;
    };
}