#include "Engine.h"

class MyGame final : public eng::IGame
{
public:
    void OnInit() override
    {
        // init game state here
        x = 100;
        vx = 120; // pixels/sec
    }

    void FixedUpdate(float dt) override
    {
        x += vx * dt;
        if (x > 900) vx = -vx;
        if (x < 100) vx = -vx;
    }

    void Render(eng::Renderer2D& r, float) override
    {
        r.DrawRectFilled(eng::RectI{ static_cast<int>(x), 200, 80, 80 }, eng::Color{ 200, 80, 80, 255 });
    }

private:
    float x = 0.0f;
    float vx = 0.0f;
};

int main(int, char**)
{
    eng::AppConfig cfg;
    cfg.title = "My2DEngine - Game";

    MyGame game;
    return eng::Run(cfg, game);
}