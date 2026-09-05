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
        const unsigned long long song_id = object[U"id"].get<unsigned long long>();
        const ScoreRecorder score_recorder{
            static_cast<ClearType>(object[U"clear"].get<int>()),
            object[U"score"].get<unsigned int>(),
            object[U"maxCombo"].get<unsigned int>(),
            object[U"perfect"].get<unsigned int>(),
            object[U"good"].get<unsigned int>(),
            object[U"miss"].get<unsigned int>()
        };
        data.scoreRecords[song_id] = score_recorder;
    }

    changeScene(SceneState::Title, 0.0s);
}

void InitializeScene::update() { }

void InitializeScene::draw() const { }
