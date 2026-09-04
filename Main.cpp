#include <Siv3D.hpp>

#include "src/Common.hpp"
#include "src/scenes/PlayScene.hpp"
#include "src/scenes/ResultScene.hpp"
#include "src/scenes/SongSelectScene.hpp"
#include "src/scenes/TitleScene.hpp"

void Main()
{
    System::SetTerminationTriggers(UserAction::CloseButtonClicked);
    Window::SetTitle(U"CHUNITHM-like Rhythm Game");
    // Window::SetFullscreen(true);
    Window::SetStyle(WindowStyle::Sizable);
    Scene::SetBackground(ColorF{ 0.04, 0.05, 0.09 });

    // 画面の登録、更新、フェード中の描画までSiv3D標準のSceneManagerに任せます。
    App app;
    app.add<TitleScene>(SceneState::Title);
    app.add<SongSelectScene>(SceneState::SongSelect);
    app.add<PlayScene>(SceneState::Play);
    app.add<ResultScene>(SceneState::Result);

    app.init(SceneState::Title, 0.0s);

    while (System::Update())
    {
        if (not app.update())
        {
            break;
        }
    }
}
