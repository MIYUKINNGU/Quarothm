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

    data.selectedSong = SongSelection{
        m_songRegistry[static_cast<unsigned long long>(data.cache[U"last_selected_song_id"].get<int32>())],
        static_cast<SongsFolderSpecies>(data.cache[U"last_selected_song_folder_id"].get<int32>()),
        static_cast<size_t>(data.cache[U"last_selected_song_index"].get<int32>()),
        static_cast<Difficulty>(data.cache[U"last_selected_difficulty"].get<int32>())
    };

    m_renderIndex = data.selectedSong.selectedSongIndex;
}

void SongSelectScene::updateImpl()
{
    auto& data = getData();
    if (KeyEscape.down())
    {
        changeScene(SceneState::Title, 0.0s);
        return;
    }
    if (KeyLeft.down())
    {
        const auto& songs_in_folder = m_songFolderRegistry.at(data.selectedSong.selectedFolderSpecies);
        if (data.selectedSong.selectedSongIndex > 0)
        {
            data.selectedSong.selectedSongIndex--;
            data.selectedSong.selectedSongData = songs_in_folder[data.selectedSong.selectedSongIndex];
            m_frameCount = Scene::FrameCount();
        }
    }
    else if (KeyRight.down())
    {
        const auto& songs_in_folder = m_songFolderRegistry.at(data.selectedSong.selectedFolderSpecies);
        if (data.selectedSong.selectedSongIndex < songs_in_folder.size() - 1)
        {
            data.selectedSong.selectedSongIndex++;
            data.selectedSong.selectedSongData = songs_in_folder[data.selectedSong.selectedSongIndex];
            m_frameCount = Scene::FrameCount();
        }
    }
    if (KeyEnter.down())
    {
        data.presentPlayData.score = 0;
        data.presentPlayData.maxCombo = 0;
        changeScene(SceneState::Play, 0.0s);
    }

    if (m_beforeFrameCount != Scene::FrameCount()) {
        m_renderIndex += (m_renderIndex - data.selectedSong.selectedSongIndex) * (std::pow(0.5, ResponsiveValue(0, Scene::FrameCount()-m_frameCount)) - 1);
        m_beforeFrameCount = Scene::FrameCount();
    }
}

void SongSelectScene::drawImpl() const
{
    const Vec2 center = Scene::CenterF();
    const auto& data = getData();

    m_bodyFont(U"楽曲選曲").draw(24.0_sc, Vec2{ 26, 15 }, Palette::White);

    double scroll_offset = 73.33333_sc;
    const SongsFolderSpecies selected_folder_species = data.selectedSong.selectedFolderSpecies;
    const SongData selected_song = data.selectedSong.selectedSongData;
    const auto& songs_in_folder = m_songFolderRegistry.at(selected_folder_species);
        
    for (size_t i = 0; i < songs_in_folder.size(); i++)
    {
        const SongData& song_data = songs_in_folder[i];

        double render_index_i = i - m_renderIndex;

        Vec2 basePos(0, 0);
        double text_x = 0;
        double baseMultipler = 2.0 / 3.0;
        if (render_index_i <= -1) {
            basePos = Vec2(-110 + 166.66666 * render_index_i, -90);
            text_x = 203.33333 * render_index_i;
        }
        else if (-1 <= render_index_i && render_index_i < 0) {
            basePos = Vec2(-110 + 166.66666 * render_index_i, -165 - 75 * render_index_i);
            baseMultipler = - std::abs(render_index_i) / 3.0 + 1;
            text_x = 203.33333 * render_index_i;
        }
        else if (0 <= render_index_i && render_index_i < 1) {
            basePos = Vec2(-110 + 240 * render_index_i, -165 + 75 * render_index_i);
            baseMultipler = - std::abs(render_index_i) / 3.0 + 1;
            text_x = 203.33333 * render_index_i;
        }
        else {
            basePos = Vec2(-36.66666 + 166.66666 * render_index_i, -90);
            text_x = 36.6666 + 166.6666 * render_index_i;
        }
    
        RoundRect{
            center.x + ResponsiveValue(basePos.x), center.y + ResponsiveValue(basePos.y),
            ResponsiveValue(220 * baseMultipler), ResponsiveValue(330 * baseMultipler),
            5
        }.draw(ColorF{ 0.12, 0.22, 0.35 });
        m_bodyFont(song_data.title).draw(
            24.0_sc * baseMultipler,
            Arg::center(Vec2{
                center.x + ResponsiveValue(text_x),
                center.y + ResponsiveValue(-0.311111111 * basePos.y + 78.6666)
            }),
            Palette::White
        );
    }

    // +35 -> 0 -> +35
    // 220 -> 330 -> 220
}
