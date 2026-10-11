#pragma once

#include <Siv3D.hpp>
#include "types/SongSelection.hpp"
#include "types/ScoreRecorder.hpp"
#include "types/SettingsStorage.hpp"

// Siv3D の SceneManager が画面の識別と共有データを管理します。
enum class SceneState
{
    Title,
    SongSelect,
    Play,
    Result,
    Initialize
};

struct GameData
{
    std::unordered_map<unsigned long long, ScoreRecorder> scoreRecords{};
    SongSelection selectedSong = SongSelection{ SongData::nullSongData, SongsFolderSpecies::Singles, 0, Difficulty::Easy };
    ScoreRecorder presentPlayData = ScoreRecorder{};
    JSON cache = JSON();
    SettingsStorage settings = SettingsStorage{};
    // FadeOut の計測値を、シーンの型に依存せず次のシーンへ共有します。
    double fadeDuration = 0.0;
};

using App = SceneManager<SceneState, GameData>;

inline void DebugLog(const String& message) {
#ifdef __DEBUG__
    Console << message;
#endif
}

[[noreturn]]
inline void ThrowError(const String& message, const String& details = U"") {
#ifdef __DEBUG__
    throw Error{ message + U"\nDetails: " + details };
#else
    throw Error{ message };
#endif
}

inline void DebugWarning(const String& message) {
#ifdef __DEBUG__
    Console << U"Warning: " << message;
#endif
}
