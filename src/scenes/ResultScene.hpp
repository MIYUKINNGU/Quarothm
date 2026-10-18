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
    const Font m_headingFont{ FontMethod::MSDF, static_cast<int32>(42_sch), Typeface::Bold };
    const Font m_bodyFont{ FontMethod::MSDF, static_cast<int32>(26_sch) };
    unsigned int m_record_score = 0;
};
