#include "TitleScene.hpp"

void TitleScene::initialize()
{
    // タイトル画面へ遷移した直後の初期化をここへ追加します。
}

void TitleScene::update()
{
    if (KeyEnter.down())
    {
        changeScene(SceneState::SongSelect, 0.0s);
    }
    
    if (KeyEscape.down())
    {
        System::Exit();
    }
}

void TitleScene::draw() const
{
    m_titleFont(U"RHYTHM GAME").drawAt(Scene::Center().movedBy(0, -45), Palette::Aqua);
    m_guideFont(U"Press Enter").drawAt(Scene::Center().movedBy(0, 45), Palette::White);
}
