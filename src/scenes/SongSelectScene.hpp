#pragma once
#include "../Common.hpp"
#include "../utillities/ResponsiveValue.hpp"

class SongSelectScene final : public App::Scene
{
public:
    SongSelectScene(const InitData& init) : IScene{ init } { initialize(); }
    void initialize();
    void update() override;
    void draw() const override;

private:
    const Font m_headingFont{ FontMethod::MSDF, 40, Typeface::Bold };
    const Font m_bodyFont{ FontMethod::MSDF, 48 };;
    int m_sceneHeight = Scene::Height();
};
