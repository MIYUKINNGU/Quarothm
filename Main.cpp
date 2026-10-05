#include <Siv3D.hpp>

#include "src/Common.hpp"
#include "src/scenes/PlayScene.hpp"
#include "src/scenes/ResultScene.hpp"
#include "src/scenes/SongSelectScene.hpp"
#include "src/scenes/TitleScene.hpp"
#include "src/scenes/InitializeScene.hpp"

void Main()
{
#ifndef __DEBUG__
    Console << U"Release build";
    Logger.disable();
#else
    Console << U"Debug build";
#endif

    System::SetTerminationTriggers(UserAction::CloseButtonClicked);
    Window::SetTitle(U"CHUNILIKE");
    // Window::SetFullscreen(true);
    Window::SetStyle(WindowStyle::Sizable);
    Scene::SetBackground(Color(U"#080616"));

    // 画面の登録、更新、フェード中の描画までSiv3D標準のSceneManagerに任せます。
    App app;
    app.add<InitializeScene>(SceneState::Initialize);
    app.add<TitleScene>(SceneState::Title);
    app.add<SongSelectScene>(SceneState::SongSelect);
    app.add<PlayScene>(SceneState::Play);
    app.add<ResultScene>(SceneState::Result);

    app.init(SceneState::Initialize, 0.0s);

    while (System::Update())
    {
        if (not app.update())
        {
            break;
        }
    }
}
