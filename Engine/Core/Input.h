#pragma once
#include <cstdint>

namespace eng::input
{
    enum class Action : uint8_t
    {
        Left,
        Right,
        Up,
        Down,
        Jump,
        Dash,
        Attack,
        Interact,
        Pause,
        DebugToggle,

        Count
    };

    // Action-based input. Keyboard-only for now (controller later).
    class Input
    {
    public:
        static void InitDefaults();

        // Call once per frame (before processing events)
        static void BeginFrame();

        // Feed SDL events here (key/window focus)
        static void ProcessEvent(void* sdlEvent); // takes SDL_Event* without exposing SDL headers

        // Call once per frame (after event polling). Updates keyboard state + computes edges.
        static void EndFrame();

        // Queries
        static bool Down(Action a);
        static bool Pressed(Action a);   // edge, not consumed
        static bool Released(Action a);  // edge, not consumed

        // Use these inside FixedUpdate so multiple fixed steps don't trigger multiple jumps.
        static bool ConsumePressed(Action a);
        static bool ConsumeReleased(Action a);

        // Convenience axes
        static float AxisX(); // -1..+1 (Left/Right)

        // Rebind (optional)
        static void BindKey(Action a, int primaryScancode, int altScancode = 0);

    private:
        Input() = delete;
    };
}