#include "InitializeScene.hpp"

void InitializeScene::initialize()
{
    GameData& data = getData();

    if (FileSystem::Exists(U"saves/play_records.json")) {
        const JSON play_records_json = JSON::Load(U"saves/play_records.json");

        for (const auto& [index, object] : play_records_json)
        {
            const unsigned long long song_id = static_cast<unsigned long long>(object[U"id"].get<int32>());
            const ScoreRecorder score_recorder{
                static_cast<ClearType>(object[U"clear"].get<int>()),
                static_cast<unsigned int>(object[U"score"].get<int32>()),
                static_cast<unsigned int>(object[U"maxCombo"].get<int32>()),
                object[U"accuracy"].get<double>(),
                static_cast<unsigned int>(object[U"perfectplus"].get<int32>()),
                static_cast<unsigned int>(object[U"perfect"].get<int32>()),
                static_cast<unsigned int>(object[U"good"].get<int32>()),
                static_cast<unsigned int>(object[U"miss"].get<int32>())
            };
            data.scoreRecords[song_id] = score_recorder;
        }
    }
    else {
        JSON records = JSON::Load(U"assets/default_play_records.json");
        records.save(U"saves/play_records.json");
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
