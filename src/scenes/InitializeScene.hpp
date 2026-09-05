#pragma once
#include "../Common.hpp"

class InitializeScene final : public App::Scene
{
public:
    InitializeScene(const InitData& init) : IScene{ init } { initialize(); }
    void initialize();
    void update() override;
    void draw() const override;

private:
    const Font m_titleFont{ FontMethod::MSDF, 56, Typeface::Bold };
    const Font m_guideFont{ 22 };
};
