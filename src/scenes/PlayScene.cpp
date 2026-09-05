#include "PlayScene.hpp"

#include "../ecs/RhythmComponents.hpp"

PlayScene::PlayScene(const InitData& init)
    : IScene{ init }
{
    initialize();
}

void PlayScene::initialize()
{
    m_state = State::Loading;
    m_loadingStopwatch.restart();
    m_elapsedSec = 0.0;
}

void PlayScene::beginPlaying()
{
    m_world.clear();
    // ここで譜面・音源などの同期ロード後に必要な初期化を行います。
    const Entity note = m_world.createEntity();
    m_world.addComponent<NoteComponent>(note, NoteComponent{ 2.0, 2, false });
    m_world.addComponent<TransformComponent>(note, TransformComponent{ Vec2{ 400, 100 } });
    m_state = State::Playing;
}

void PlayScene::update()
{
    switch (m_state)
    {
    case State::Loading:
        // 現時点では短時間の同期ロードを想定します。
        if (m_loadingStopwatch.sF() >= 0.45)
        {
            beginPlaying();
        }
        break;

    case State::Playing:
        m_elapsedSec += Scene::DeltaTime();
        // NoteMoveSystem相当: Noteを持つEntityの位置を一括更新します。
        m_world.each<TransformComponent>([](const Entity, TransformComponent& transform)
        {
            transform.position.y += 180.0 * Scene::DeltaTime();
        });

        if (KeyR.down() || m_elapsedSec >= 10.0)
        {
            getData().presentPlayData.score = 987654;
            getData().presentPlayData.maxCombo = 123;
            m_state = State::Finished;
        }
        break;

    case State::Finished:
        changeScene(SceneState::Result, 0.25s);
        break;
    }
}

void PlayScene::draw() const
{
    if (m_state == State::Loading)
    {
        m_font(U"NOW LOADING...").drawAt(Scene::Center(), Palette::White);
        return;
    }

    Rect{ 200, 80, 400, 500 }.drawFrame(3, Palette::Aqua);
    Line{ 200, 500, 600, 500 }.draw(4, Palette::Yellow);
    m_world.each<TransformComponent>([](const Entity, const TransformComponent& transform)
    {
        Circle{ transform.position, 18 }.draw(Palette::Hotpink);
    });
    m_font((m_state == State::Finished) ? U"FINISHED" : U"PLAY  R: Finish").draw(20, 20);
}
