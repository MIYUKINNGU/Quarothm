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

    const double scale = Scene::Height() / 600.0;

    m_bodyFont(U"楽曲選曲").draw(24.0 * scale, Vec2{ 26, 15 }, Palette::White);
    RoundRect{ center.x - 110 * scale, center.y - 165 * scale, 220 * scale, 330 * scale, 5 }.draw(ColorF{ 0.12, 0.22, 0.35 });
    m_bodyFont(getData().selectedSong).draw(24.0 * scale, Arg::center(Vec2{ center.x, center.y + 130 * scale }), Palette::White);
}
