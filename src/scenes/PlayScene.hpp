#pragma once
#include "../Common.hpp"
#include "../ecs/EcsWorld.hpp"
#include "../types/AbstractScene.hpp"
#include "../utilities/ResponsiveValue.hpp"

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

    const ResponsiveValue lane_l = {-350_scw, };
    const Font m_font{ 22 };
    mutable EcsWorld m_world;
    Stopwatch m_loadingStopwatch{ StartImmediately::No };
    State m_state = State::Loading;
    double m_elapsedSec = 0.0;
};
