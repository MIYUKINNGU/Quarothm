#include "SongSelectScene.hpp"

void SongSelectScene::initialize()
{
    // 選曲リストや選択カーソルの初期化はここへ追加します。
}

void SongSelectScene::update()
{
    if (KeyEscape.down())
    {
        changeScene(SceneState::Title, 0.0s);
        return;
    }
    if (KeyEnter.down())
    {
        auto& data = getData();
        data.score = 0;
        data.maxCombo = 0;
        changeScene(SceneState::Play, 0.0s);
    }
}

void SongSelectScene::draw() const
{
    const Vec2 center = Scene::CenterF();

    m_bodyFont(U"楽曲選曲").draw(24.0_sc, Vec2{ 26, 15 }, Palette::White);
    RoundRect{ center.x - 110_sc, center.y - 165_sc, 220_sc, 330_sc, 5 }.draw(ColorF{ 0.12, 0.22, 0.35 });
    m_bodyFont(getData().selectedSong).draw(24.0_sc, Arg::center(Vec2{ center.x, center.y + 130_sc }), Palette::White);
}
