#pragma once
#include "../Common.hpp"

class TitleScene final : public App::Scene
{
public:
    TitleScene(const InitData& init) : IScene{ init } { initialize(); }
    void initialize();
    void update() override;
    void draw() const override;

private:
    const Font m_titleFont{ FontMethod::MSDF, 56, Typeface::Bold };
    const Font m_guideFont{ 22 };
};
