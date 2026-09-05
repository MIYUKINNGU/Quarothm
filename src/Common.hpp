#pragma once

#include <Siv3D.hpp>
#include "types/SongSelection.hpp"
#include "types/ScoreRecorder.hpp"

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
    SongSelection selectedSong = SongSelection{ SongData::nullSongData, SongsFolderSpecies::Singles, Difficulty::Easy };
    ScoreRecorder presentPlayData = ScoreRecorder{};
};

using App = SceneManager<SceneState, GameData>;

inline void DebugLog(const std::string& message) {
#ifdef __DEBUG__
    std::cout << message << std::endl;
#endif
};

inline void ThrowError(const String& message, const String& details = U"") {
#ifdef __DEBUG__
    throw Error{ message + U"\nDetails: " + details };
#else
    throw Error{ message };
#endif
}

inline void DebugWarning(const std::string& message) {
#ifdef __DEBUG__
    std::cerr << "Warning: " << message << std::endl;
#endif
}