#pragma once
#include "../Common.hpp"
#include "../ecs/EcsWorld.hpp"

class PlayScene final : public App::Scene
{
public:
    PlayScene(const InitData& init);
    void initialize();
    void update() override;
    void draw() const override;

private:
    enum class State
    {
        Loading,
        Playing,
        Finished,
    };

    void beginPlaying();

    const Font m_font{ 22 };
    mutable EcsWorld m_world;
    Stopwatch m_loadingStopwatch{ StartImmediately::No };
    State m_state = State::Loading;
    double m_elapsedSec = 0.0;
};
