#include "Engine.h"

class MyGame final : public eng::IGame
{
public:
    void OnInit() override
    {
        // Ground: center at (640, 680), size 1200x60
        ground = world.CreateStaticBoxPx(640.0f, 680.0f, 1200.0f, 60.0f);

        // Dynamic box: center at (640, 100), size 80x80
        box = world.CreateDynamicBoxPx(640.0f, 100.0f, 80.0f, 80.0f);
    }

    void FixedUpdate(float dt) override
    {
        world.Step(dt, 4);
    }

    void Render(eng::Renderer2D& r, float) override
    {
        // Draw ground (static)
        r.DrawRectFilled(eng::RectI{ 40, 650, 1200, 60 }, eng::Color{ 70, 70, 80, 255 });

        // Draw dynamic box (no rotation yet)
        eng::Vec2f p = world.GetPositionPx(box);
        r.DrawRectFilled(
            eng::RectI{ (int)(p.x - 40), (int)(p.y - 40), 80, 80 },
            eng::Color{ 200, 80, 80, 255 }
        );
    }

private:
    eng::physics::PhysicsWorld world;
    eng::physics::Body ground{};
    eng::physics::Body box{};
};

int main(int, char**)
{
    eng::AppConfig cfg;
    cfg.title = "My2DEngine - Hybrid Test";

    MyGame game;
    return eng::Run(cfg, game);
}