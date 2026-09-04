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
    m_headingFont(U"SONG SELECT").drawAt(400, 100, Palette::Aqua);
    RoundRect{ 180, 180, 440, 100, 12 }.draw(ColorF{ 0.12, 0.22, 0.35 });
    m_bodyFont(getData().selectedSong).drawAt(400, 230);
    m_bodyFont(U"Enter: Start    Esc: Back").drawAt(400, 400, Palette::White);
}
