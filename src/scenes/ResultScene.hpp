#pragma once
#include "../Common.hpp"
#include "../types/AbstractScene.hpp"
#include "../utilities/ResponsiveValue.hpp"

class ResultScene final : public AbstractScene<ResultScene>
{
public:
    using AbstractScene<ResultScene>::AbstractScene;
    void initialize();
    void updateImpl();
    void drawImpl() const;

private:
    const Font m_headingFont{ FontMethod::MSDF, 42, Typeface::Bold };
    const Font m_bodyFont{ 26 };
};
