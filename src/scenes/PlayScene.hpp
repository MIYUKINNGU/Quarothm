#pragma once
#include "../Common.hpp"
#include "../ecs/EcsWorld.hpp"
#include "../types/AbstractScene.hpp"
#include "../utilities/ResponsiveValue.hpp"
#include "../types/NoteType.hpp"
#include "../utilities/FilePath.hpp"
#include "../types/ChartData.hpp"
#include "../types/JudgementType.hpp"
#include <thread>
#include <chrono>
#include <future>

// DirectXのSALがGCC16のregex干渉している　←うんち
#ifdef __GLIBCXX__
#undef __pre
#endif
#include <regex>

class PlayScene final : public AbstractScene<PlayScene>
{
public:
    using AbstractScene<PlayScene>::AbstractScene;
    void initialize();
    void updateImpl();
    void drawImpl() const;

private:
    enum class State
    {
        Loading,
        Playing,
        Finished,
    };

    void beginPlaying();
    std::vector<NoteComponent*>& getCurrentLane();
    size_t& getCurrentLaneReadingIdx();
    JudgementType judgeNote(NoteComponent* note_component, double perfect_plus_threshold = 16.66, double perfect_threshold = 45.0, double good_threshold = 100.0, double miss_threshold = 150.0);
    bool isNoteLate(NoteComponent* note_component);
    void processNoteMiss(NoteComponent* note_component, size_t& index);
    void processNormalJudged(NoteComponent* note_component, size_t& index);
    void processJudging(NoteComponent* note_component, size_t& index);
    void increaseJudgeCount();

    std::vector<std::unique_ptr<NoteComponent>> noteStorage;
    size_t reading_idx_left = 0;
    size_t reading_idx_right = 0;
    size_t reading_idx_up = 0;
    size_t reading_idx_down = 0;
    bool starting_point_tapped = false;
    ChartData m_chartData;
    Audio m_song_audio;
    int32 m_currentLane = 0;
    const Font m_font{ FontMethod::MSDF, 22 };
    mutable EcsWorld m_world;
    std::future<void> m_future;
    State m_state = State::Loading;
    double m_elapsedMilliSec = 0.0;
    JudgementType m_recentJudgement = JudgementType::None;
    double m_showJudgeTime = 0;
    const Texture normal_left{ U"assets/images/notes/left_normal.png" };
    const Texture normal_right{ U"assets/images/notes/right_normal.png" };
    const Texture normal_up{ U"assets/images/notes/up_normal.png" };
    const Texture normal_down{ U"assets/images/notes/down_normal.png" };
    const std::unordered_map<int32, double> rot_map = {
        {0, 0.0},
        {1, std::numbers::pi},
        {2, std::numbers::pi / 2.0},
        {3, 3.0 * std::numbers::pi / 2.0}
    };
    double m_renderRot = 0.0;
    double m_renderJudgeSize = 1.0;
    int32 m_beforeFrameCount = 0;
    int32 m_frameCount = 0;
    int32 m_judgeFrameCount = 0;
    unsigned int combo = 0;
};
