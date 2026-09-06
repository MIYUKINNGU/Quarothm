#pragma once
#include "../Common.hpp"
#include "../utilities/ResponsiveValue.hpp"
#include <algorithm>
#include "../types/AbstractScene.hpp"
#include <cmath>

class SongSelectScene final : public AbstractScene<SongSelectScene>
{
public:
    using AbstractScene<SongSelectScene>::AbstractScene;
    void initialize();
    void updateImpl();
    void drawImpl() const;

private:
    const Font m_headingFont{ FontMethod::MSDF, 40, Typeface::Bold };
    const Font m_bodyFont{ FontMethod::MSDF, 48 };
    int m_sceneHeight = Scene::Height();
    std::vector<SongData> m_songList{};
    std::unordered_map<unsigned long long, SongData> m_songRegistry;
    std::unordered_map<SongsFolderSpecies, std::vector<SongData>> m_songFolderRegistry;
    double m_renderIndex;
    unsigned long long m_frameCount = 0;
    unsigned long long m_beforeFrameCount = 0;
};
