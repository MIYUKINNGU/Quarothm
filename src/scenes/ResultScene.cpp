#include "ResultScene.hpp"

void ResultScene::initialize()
{
    // リザルト表示用の集計・演出の初期化をここへ追加します。
}

void ResultScene::update()
{
    if (KeyEnter.down()) changeScene(SceneState::SongSelect, 0.0s);
    else if (KeyEscape.down()) changeScene(SceneState::Title, 0.0s);
}

void ResultScene::draw() const
{
    m_headingFont(U"RESULT").drawAt(400, 110, Palette::Yellow);
    m_bodyFont(U"SCORE: " + Format(getData().presentPlayData.score)).drawAt(400, 220);
    m_bodyFont(U"MAX COMBO: " + Format(getData().presentPlayData.maxCombo)).drawAt(400, 270);
    m_bodyFont(U"Enter: Song Select    Esc: Title").drawAt(400, 420);
}
