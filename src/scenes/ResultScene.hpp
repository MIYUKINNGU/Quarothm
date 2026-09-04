#pragma once
#include "../Common.hpp"

class ResultScene final : public App::Scene
{
public:
    ResultScene(const InitData& init) : IScene{ init } { initialize(); }
    void initialize();
    void update() override;
    void draw() const override;

private:
    const Font m_headingFont{ FontMethod::MSDF, 42, Typeface::Bold };
    const Font m_bodyFont{ 26 };
};
