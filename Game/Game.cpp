#include "Engine.h"

class MyGame final : public eng::IGame 
{
public:

    bool showDebug = true;

    void OnInit() override
    {
        map.Resize(30, 14);

        // flat floor
        for (int x = 0; x < map.width; ++x)
            map.Set(x, 12, eng::world::ColliderType::Solid);

        // 45° ramp up to the right (diagonal staircase of slope tiles)
        map.Set(5, 11, eng::world::ColliderType::Slope45_Up);
        map.Set(6, 10, eng::world::ColliderType::Slope45_Up);
        map.Set(7, 9, eng::world::ColliderType::Slope45_Up);
        map.Set(8, 8, eng::world::ColliderType::Slope45_Up);

        // platform after ramp
        map.Set(9, 8, eng::world::ColliderType::Solid);
        map.Set(10, 8, eng::world::ColliderType::Solid);
        map.Set(11, 8, eng::world::ColliderType::Solid);

        // controller start
        player.SetPositionPx({ 200.0f, 300.0f });
    }

    void FixedUpdate(float dt) override
    {
        float moveX = eng::input::Input::AxisX();
        bool jump = eng::input::Input::ConsumePressed(eng::input::Action::Jump);

        player.FixedUpdate(dt, moveX, jump, map);

        if (eng::input::Input::ConsumePressed(eng::input::Action::DebugToggle))
            showDebug = !showDebug;
    }

    void Render(eng::Renderer2D& r, float) override
    {
        if (showDebug)
            player.DebugDraw(r, map);
    }

private:
    eng::world::Tilemap map;
    eng::gameplay::CharacterController player;
};

int main(int, char**)
{
    eng::AppConfig cfg;
    cfg.title = "Slope + Capsule Test";
    return eng::Run(cfg, *new MyGame()); // quick test; replace with stack instance if you prefer
}