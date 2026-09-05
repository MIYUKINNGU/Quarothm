#include "InitializeScene.hpp"

void InitializeScene::initialize()
{
    const JSON play_records_json = JSON::Load(U"saves/play_records.json");

	if (not play_records_json)
	{
		ThrowError(U"Failed to load.", U"Failed to load `saves/play_records.json`");
	}

    GameData& data = getData();

    for (const auto& [index, object] : play_records_json)
    {
        const unsigned long long song_id = static_cast<unsigned long long>(object[U"id"].get<int32>());
        const ScoreRecorder score_recorder{
            static_cast<ClearType>(object[U"clear"].get<int>()),
            static_cast<unsigned int>(object[U"score"].get<int32>()),
            static_cast<unsigned int>(object[U"maxCombo"].get<int32>()),
            static_cast<unsigned int>(object[U"perfect"].get<int32>()),
            static_cast<unsigned int>(object[U"good"].get<int32>()),
            static_cast<unsigned int>(object[U"miss"].get<int32>())
        };
        data.scoreRecords[song_id] = score_recorder;
    }

    if (FileSystem::Exists(U"saves/cache_data.json")) {
        data.cache = JSON::Load(U"saves/cache_data.json");
    }
    else {
        data.cache = JSON::Load(U"assets/default_cache_data.json");
        data.cache.save(U"saves/cache_data.json");
    }
}

void InitializeScene::updateImpl() {
    changeScene(SceneState::Title, 0.0s);
}

void InitializeScene::drawImpl() const { }
