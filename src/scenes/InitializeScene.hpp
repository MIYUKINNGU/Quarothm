#pragma once
#include "../Common.hpp"
#include "../types/AbstractScene.hpp"

class InitializeScene final : public AbstractScene<InitializeScene>
{
public:
    using AbstractScene<InitializeScene>::AbstractScene;
    void initialize();
    void updateImpl();
    void drawImpl() const;

private:
    const Font m_titleFont{ FontMethod::MSDF, 56, Typeface::Bold };
    const Font m_guideFont{ 22 };
};
