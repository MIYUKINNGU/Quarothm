#pragma once

#include <Siv3D.hpp>

// Siv3D の SceneManager が画面の識別と共有データを管理します。
enum class SceneState
{
    Title,
    SongSelect,
    Play,
    Result,
};

struct GameData
{
    String selectedSong = U"Sample Song";
    int32 score = 0;
    int32 maxCombo = 0;
};

using App = SceneManager<SceneState, GameData>;
