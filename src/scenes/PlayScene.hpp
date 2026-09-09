#pragma once
#include "../Common.hpp"
#include "../ecs/EcsWorld.hpp"
#include "../types/AbstractScene.hpp"
#include "../utilities/ResponsiveValue.hpp"
#include "../types/NoteType.hpp"
#include "../utilities/FilePath.hpp"
#include "../types/ChartData.hpp"
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

    std::vector<std::unique_ptr<NoteComponent>> noteStorage;
    size_t reading_idx_left = 0;
    size_t reading_idx_right = 0;
    size_t reading_idx_up = 0;
    size_t reading_idx_down = 0;
    ChartData m_chartData;
    Audio m_song_audio;
    int32 m_currentLane = 0;
    const Font m_font{ 22 };
    mutable EcsWorld m_world;
    std::future<void> m_future;
    State m_state = State::Loading;
    double m_elapsedMilliSec = 0.0;
};
