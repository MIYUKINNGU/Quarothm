#include "SongSelectScene.hpp"

void SongSelectScene::initialize()
{
    const JSON song_list_json = JSON::Load(U"assets/songs/songs_list.json");

	if (not song_list_json)
	{
		ThrowError(U"Failed to load.", U"Failed to load `assets/songs/songs_list.json`");
	}

    for (const auto& [index, object] : song_list_json)
    {
        const unsigned long long song_id = static_cast<unsigned long long>(object[U"id"].get<int32>());
        const SongData song_data{
            song_id,
            object[U"title"].getString(),
            object[U"artist"].getString(),
            object[U"path"].getString(),
        };
        m_songRegistry[song_id] = song_data;
    }

    const JSON songsFolder_list_json = JSON::Load(U"assets/songs/songsfolder_list.json");

	if (not songsFolder_list_json)
	{
		ThrowError(U"Failed to load.", U"Failed to load `assets/songs/songsfolder_list.json`");
	}

    for (const auto& [index, object] : songsFolder_list_json)
    {
        const SongsFolderSpecies folder_species = static_cast<SongsFolderSpecies>(object[U"id"].get<int32>());
        for (const auto& [index, song_id_raw] : object[U"song_ids"])
        {
            const unsigned long long song_id = static_cast<unsigned long long>(song_id_raw.get<int32>());

            if (m_songRegistry.find(song_id) != m_songRegistry.end() &&
                m_songFolderRegistry.find(folder_species) != m_songFolderRegistry.end())
            {
                m_songFolderRegistry[folder_species].push_back(m_songRegistry[song_id]);
            }
            else if (m_songRegistry.find(song_id) != m_songRegistry.end())
            {
                m_songFolderRegistry[folder_species] = { m_songRegistry[song_id] };
            }
            else {
                DebugWarning(U"Song (id: " + Format(song_id) + U") not found in registry.");
            }
        }
    }

    auto& data = getData();

    data.selectedSong = SongSelection{ m_songRegistry[static_cast<unsigned long long>(data.cache[U"last_selected_song_id"].get<int32>())], static_cast<SongsFolderSpecies>(data.cache[U"last_selected_song_folder_id"].get<int32>()), static_cast<Difficulty>(data.cache[U"last_selected_difficulty"].get<int32>()) };
}

void SongSelectScene::updateImpl()
{
    if (KeyEscape.down())
    {
        changeScene(SceneState::Title, 0.0s);
        return;
    }
    if (KeyEnter.down())
    {
        auto& data = getData();
        data.presentPlayData.score = 0;
        data.presentPlayData.maxCombo = 0;
        changeScene(SceneState::Play, 0.0s);
    }
}

void SongSelectScene::drawImpl() const
{
    const Vec2 center = Scene::CenterF();

    m_bodyFont(U"楽曲選曲").draw(24.0_sc, Vec2{ 26, 15 }, Palette::White);
    RoundRect{ center.x - 110_sc, center.y - 165_sc, 220_sc, 330_sc, 5 }.draw(ColorF{ 0.12, 0.22, 0.35 });
    m_bodyFont(getData().selectedSong.selectedSongData.title).draw(24.0_sc, Arg::center(Vec2{ center.x, center.y + 130_sc }), Palette::White);
}
