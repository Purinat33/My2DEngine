#include "pch.h"
#include "Core/Input.h"

#include <array>
#include <algorithm>

#include <SDL2/SDL.h>

namespace eng::input
{
    static constexpr size_t ActionCount = (size_t)Action::Count;

    struct Binding
    {
        SDL_Scancode primary = SDL_SCANCODE_UNKNOWN;
        SDL_Scancode alt = SDL_SCANCODE_UNKNOWN;
    };

    static std::array<Binding, ActionCount> s_bindings{};

    static std::array<uint8_t, ActionCount> s_curr{};     // 0/1
    static std::array<uint8_t, ActionCount> s_prev{};     // 0/1
    static std::array<uint8_t, ActionCount> s_pressed{};  // edge, can be consumed
    static std::array<uint8_t, ActionCount> s_released{}; // edge, can be consumed

    static bool s_hasFocus = true;
    static float s_axisX = 0.0f;

    static bool KeyDown(const uint8_t* kb, SDL_Scancode sc)
    {
        return (sc != SDL_SCANCODE_UNKNOWN) ? (kb[sc] != 0) : false;
    }

    void Input::InitDefaults()
    {
        BindKey(Action::Left, SDL_SCANCODE_A, SDL_SCANCODE_LEFT);
        BindKey(Action::Right, SDL_SCANCODE_D, SDL_SCANCODE_RIGHT);
        BindKey(Action::Up, SDL_SCANCODE_W, SDL_SCANCODE_UP);
        BindKey(Action::Down, SDL_SCANCODE_S, SDL_SCANCODE_DOWN);

        BindKey(Action::Jump, SDL_SCANCODE_SPACE);
        BindKey(Action::Dash, SDL_SCANCODE_LSHIFT, SDL_SCANCODE_RSHIFT);
        BindKey(Action::Attack, SDL_SCANCODE_J);
        BindKey(Action::Interact, SDL_SCANCODE_E);

        BindKey(Action::Pause, SDL_SCANCODE_ESCAPE);
        BindKey(Action::DebugToggle, SDL_SCANCODE_F1);
    }

    void Input::BeginFrame()
    {
        // clear edges for this new frame; they'll be recomputed in EndFrame()
        std::fill(s_pressed.begin(), s_pressed.end(), 0);
        std::fill(s_released.begin(), s_released.end(), 0);

        s_axisX = 0.0f;
    }

    void Input::ProcessEvent(void* sdlEvent)
    {
        SDL_Event& e = *static_cast<SDL_Event*>(sdlEvent);

        if (e.type == SDL_WINDOWEVENT)
        {
            if (e.window.event == SDL_WINDOWEVENT_FOCUS_GAINED)
                s_hasFocus = true;
            else if (e.window.event == SDL_WINDOWEVENT_FOCUS_LOST)
                s_hasFocus = false;
        }
    }

    void Input::EndFrame()
    {
        // Snapshot previous
        s_prev = s_curr;
        std::fill(s_curr.begin(), s_curr.end(), 0);

        if (!s_hasFocus)
        {
            // no input while unfocused
            return;
        }

        SDL_PumpEvents();
        int numKeys = 0;
        const uint8_t* kb = SDL_GetKeyboardState(&numKeys);

        for (size_t i = 0; i < ActionCount; ++i)
        {
            const Binding& b = s_bindings[i];
            bool down = KeyDown(kb, b.primary) || KeyDown(kb, b.alt);
            s_curr[i] = down ? 1 : 0;
        }

        // Edges
        for (size_t i = 0; i < ActionCount; ++i)
        {
            const bool c = (s_curr[i] != 0);
            const bool p = (s_prev[i] != 0);

            if (c && !p) s_pressed[i] = 1;
            if (!c && p) s_released[i] = 1;
        }

        // AxisX
        float ax = 0.0f;
        if (Down(Action::Left))  ax -= 1.0f;
        if (Down(Action::Right)) ax += 1.0f;
        s_axisX = ax;
    }

    bool Input::Down(Action a)
    {
        return s_curr[(size_t)a] != 0;
    }

    bool Input::Pressed(Action a)
    {
        return s_pressed[(size_t)a] != 0;
    }

    bool Input::Released(Action a)
    {
        return s_released[(size_t)a] != 0;
    }

    bool Input::ConsumePressed(Action a)
    {
        const size_t i = (size_t)a;
        const bool v = (s_pressed[i] != 0);
        s_pressed[i] = 0;
        return v;
    }

    bool Input::ConsumeReleased(Action a)
    {
        const size_t i = (size_t)a;
        const bool v = (s_released[i] != 0);
        s_released[i] = 0;
        return v;
    }

    float Input::AxisX()
    {
        return s_axisX;
    }

    void Input::BindKey(Action a, int primaryScancode, int altScancode)
    {
        Binding& b = s_bindings[(size_t)a];
        b.primary = (primaryScancode != 0) ? (SDL_Scancode)primaryScancode : SDL_SCANCODE_UNKNOWN;
        b.alt = (altScancode != 0) ? (SDL_Scancode)altScancode : SDL_SCANCODE_UNKNOWN;
    }
}