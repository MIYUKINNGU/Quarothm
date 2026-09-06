#include "PlayScene.hpp"

#include "../ecs/RhythmComponents.hpp"

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
    m_world.addComponent<NoteComponent>(note, NoteComponent{ 2.0, 2, 3, false });
    m_world.addComponent<TransformComponent>(note, TransformComponent{ Vec2{ 400, 100 } });
    m_state = State::Playing;
}

void PlayScene::updateImpl()
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

void PlayScene::drawImpl() const
{
    if (m_state == State::Loading)
    {
        m_font(U"NOW LOADING...").drawAt(Scene::Center(), Palette::White);
        return;
    }

    Point center = Scene::Center();

    Rect{ center.x - 350_scw, -10_sch, 700_scw, Scene::Height() + 20_sch }.drawFrame(3_scw, Palette::White);
    for (int i = 1; i < 12; i++) {
        Line{
            center.x - 350_scw + 58.3333333333_scw * i, -10_sch,
            center.x - 350_scw + 58.3333333333_scw * i, Scene::Height() + 10_sch
        }.draw(1_scw * (2 != i && i != 10 ? ((i - 3 ^ i - 2) + 1) / 2 : 2), Palette::Gray);
    }
    Line{ center.x - 350_scw, Scene::Height() - 100_sch, center.x + 350_scw, Scene::Height() - 100_sch }.draw(4_scw, Palette::Yellow);
    m_world.each<TransformComponent>([](const Entity, const TransformComponent& transform)
    {
        Circle{ transform.position, 18 }.draw(Palette::Hotpink);
    });
    m_font((m_state == State::Finished) ? U"FINISHED" : U"PLAY  R: Finish").draw(20, 20);
}
