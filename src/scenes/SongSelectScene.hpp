#pragma once
#include "../Common.hpp"
#include "../utilities/ResponsiveValue.hpp"
#include <algorithm>
#include "../types/AbstractScene.hpp"
#include <cmath>
#include "../utilities/FilePath.hpp"

class SongSelectScene final : public AbstractScene<SongSelectScene>
{
public:
    using AbstractScene<SongSelectScene>::AbstractScene;
    void initialize();
    void updateImpl();
    void updateSettings();
    void updateSongSelect();
    void drawImpl() const;
    void store2Cache();
    void selectRightSideSong();
    void selectLeftSideSong();
    void updatePreviewAudio();

private:
    const Font m_headingFont{ FontMethod::MSDF, 40, Typeface::Bold };
    const Font m_bodyFont{ FontMethod::MSDF, 48 };
    int m_sceneHeight = Scene::Height();
    std::vector<SongData> m_songList{};
    std::unordered_map<unsigned long long, SongData> m_songRegistry;
    std::unordered_map<SongsFolderSpecies, std::vector<SongData>> m_songFolderRegistry;
    std::unordered_map<unsigned long long, Texture> m_jacketTextures;
    double m_renderIndex;
    unsigned long long m_frameCount = 0;
    unsigned long long m_beforeFrameCount = 0;
    std::vector<unsigned long long> m_unlocked_hidden;
    bool m_settingsOpen = false;
    double m_settingsProgress = 0.0;
    int32 m_settingsSelection = 0;
    Audio m_previewAudio;
    unsigned long long m_previewSongId = 0;
    bool m_previewInitialized = false;
    double m_previewStartSec = 0.0;
    double m_previewLengthSec = 0.0;
    double m_previewRestartDelaySec = 0.0;
};
