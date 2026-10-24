#pragma once
#include "../Common.hpp"
#include "../utilities/ResponsiveValue.hpp"
#include "../types/AbstractScene.hpp"

class TitleScene final : public AbstractScene<TitleScene>
{
public:
    using AbstractScene<TitleScene>::AbstractScene;
    void initialize();
    void updateImpl();
    void drawImpl() const;

private:
    const Font m_titleFont{ FontMethod::MSDF, 56, Typeface::Bold };
    const Font m_guideFont{ 22 };
    bool m_creditsOpen = false;
    double m_creditsProgress = 0.0;
	bool m_isFullScreen = true;
};
