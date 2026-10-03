#include "SongSelectScene.hpp"

template <typename T>
inline bool contains_vector(const std::vector<T>& vec, const T& value) {
    return std::find(vec.begin(), vec.end(), value) != vec.end();
}

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
        std::vector<String> additional_labels;
        for (const auto& [index, label] : object[U"additional_labels"])
        {
            additional_labels.push_back(label.getString());
        }
        const SongData song_data{
            song_id,
            object[U"title"].getString(),
            object[U"artist"].getString(),
            object[U"path"].getString(),
            additional_labels
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

    for (const auto& [index, id] : data.cache[U"unlocked_hidden_songs"]) {
        m_unlocked_hidden.push_back(id.get<int32>());
    }

    m_renderIndex = data.selectedSong.selectedSongIndex;
}

void SongSelectScene::store2Cache()
{
    auto& data = getData();
    data.cache[U"last_selected_song_id"] = static_cast<int32>(data.selectedSong.selectedSongData.id);
    data.cache[U"last_selected_song_folder_id"] = static_cast<int32>(data.selectedSong.selectedFolderSpecies);
    data.cache[U"last_selected_song_index"] = static_cast<int32>(data.selectedSong.selectedSongIndex);
    data.cache[U"last_selected_difficulty"] = static_cast<int32>(data.selectedSong.selectedDifficulty);
}

void SongSelectScene::selectRightSideSong()
{
    auto& data = getData();
    const auto& songs_in_folder = m_songFolderRegistry.at(data.selectedSong.selectedFolderSpecies);

    size_t before_index = data.selectedSong.selectedSongIndex;
    do {
        data.selectedSong.selectedSongIndex++;
    } while (data.selectedSong.selectedSongIndex < songs_in_folder.size()
         && contains_vector<String>(songs_in_folder[data.selectedSong.selectedSongIndex].additionalLabels, U"default_hidden")
         && !contains_vector<unsigned long long>(m_unlocked_hidden, songs_in_folder[data.selectedSong.selectedSongIndex].id));
    
    if (data.selectedSong.selectedSongIndex >= songs_in_folder.size()) data.selectedSong.selectedSongIndex = before_index;
    data.selectedSong.selectedSongData = songs_in_folder[data.selectedSong.selectedSongIndex];
    data.selectedSong.selectedSongId =  data.selectedSong.selectedSongData.id;
    m_frameCount = Scene::FrameCount();
    
} // ここらへん動作確認する

void SongSelectScene::selectLeftSideSong()
{
    auto& data = getData();
    const auto& songs_in_folder = m_songFolderRegistry.at(data.selectedSong.selectedFolderSpecies);
    size_t before_index = data.selectedSong.selectedSongIndex;
    for (const auto& [index, id] : data.cache[U"unlocked_hidden_songs"]) {
        m_unlocked_hidden.push_back(static_cast<unsigned long long>(id.get<int32>()));
    }
    if (data.selectedSong.selectedSongIndex > 0) do {
        data.selectedSong.selectedSongIndex--;
    } while (data.selectedSong.selectedSongIndex > 0
         && contains_vector<String>(songs_in_folder[data.selectedSong.selectedSongIndex].additionalLabels, U"default_hidden")
         && !contains_vector<unsigned long long>(m_unlocked_hidden, songs_in_folder[data.selectedSong.selectedSongIndex].id));

    if (contains_vector<String>(songs_in_folder[data.selectedSong.selectedSongIndex].additionalLabels, U"default_hidden")
         && !contains_vector<unsigned long long>(m_unlocked_hidden, songs_in_folder[data.selectedSong.selectedSongIndex].id)) {
        data.selectedSong.selectedSongIndex = before_index;
    }
    data.selectedSong.selectedSongData = songs_in_folder[data.selectedSong.selectedSongIndex];
    data.selectedSong.selectedSongId =  data.selectedSong.selectedSongData.id;
    m_frameCount = Scene::FrameCount();
}

void SongSelectScene::updateImpl()
{
    auto& data = getData();
    if (KeyEscape.down())
    {
        changeScene(SceneState::Title, 0.0s);
        return;
    }
    if (KeyLeft.down() || KeyA.down())
    {
        selectLeftSideSong();
    }
    else if (KeyRight.down() || KeyD.down())
    {
        selectRightSideSong();
    }
    else if (KeyUp.down() || KeyW.down()) {
        switch (data.selectedSong.selectedDifficulty) {
            case Difficulty::Easy:
                data.selectedSong.selectedDifficulty = Difficulty::Normal;
                break;
            case Difficulty::Normal:
                data.selectedSong.selectedDifficulty = Difficulty::Expert;
                break;
        }
    }
    else if (KeyDown.down() || KeyS.down()) {
        switch (data.selectedSong.selectedDifficulty) {
            case Difficulty::Expert:
                data.selectedSong.selectedDifficulty = Difficulty::Normal;
                break;
            case Difficulty::Normal:
                data.selectedSong.selectedDifficulty = Difficulty::Easy;
                break;
        }
    }
    if (KeyEnter.down() || KeyK.down())
    {
        data.presentPlayData.score = 0;
        data.presentPlayData.maxCombo = 0;
        std::vector<String> additional_labels = data.selectedSong.selectedSongData.additionalLabels;
        if (contains_vector<String>(additional_labels, U"only_expert") && data.selectedSong.selectedDifficulty != Difficulty::Expert) {
            data.selectedSong.selectedDifficulty = Difficulty::Expert;
        }
        store2Cache();
        changeScene(SceneState::Play, 0.0s);
    }

    if (m_beforeFrameCount != Scene::FrameCount()) {
        m_renderIndex += (m_renderIndex - data.selectedSong.selectedSongIndex) * (std::pow(0.5, ResponsiveValue(0, static_cast<double>(Scene::FrameCount()-m_frameCount))) - 1);
        m_beforeFrameCount = Scene::FrameCount();
    }
    store2Cache();
}

void SongSelectScene::drawImpl() const
{
    const Vec2 center = Scene::CenterF();
    const auto& data = getData();

    m_bodyFont(U"楽曲選曲").draw(24.0_sch, Vec2{ 26_sch, 15_sch }, Palette::White);

    double scroll_offset = 73.33333_sch;
    const SongsFolderSpecies selected_folder_species = data.selectedSong.selectedFolderSpecies;
    const SongData selected_song = data.selectedSong.selectedSongData;
    const auto& songs_in_folder = m_songFolderRegistry.at(selected_folder_species);
    
    int view_index = 0;
    int skipped_count = 0;
    for (size_t i = 0; i < songs_in_folder.size(); i++)
    {
        const SongData& song_data = songs_in_folder[i];

        double render_index_i = view_index - m_renderIndex + skipped_count;

        if (contains_vector<String>(song_data.additionalLabels, U"default_hidden")
         && !contains_vector<unsigned long long>(m_unlocked_hidden, song_data.id)) {
            skipped_count++;
            continue;
        }

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

        Color background_color;
        Color color;
        switch (data.selectedSong.selectedDifficulty) {
            case Difficulty::Easy:
                background_color = Palette::Lightskyblue;
                color = Palette::Black;
                break;
            case Difficulty::Normal:
                background_color = Palette::Lightgreen;
                color = Palette::Black;
                break;
            case Difficulty::Expert:
                background_color = Palette::Orangered;
                color = Palette::White;
                break;
        }
        if (contains_vector<String>(song_data.additionalLabels, U"only_expert")) {
            background_color = Palette::Orangered;
            color = Palette::White;
        }
    
        RoundRect{
            center.x + ResponsiveValue(basePos.x), center.y + ResponsiveValue(basePos.y),
            ResponsiveValue(220 * baseMultipler), ResponsiveValue(330 * baseMultipler),
            5
        }.draw(background_color);
        m_bodyFont(song_data.title).draw(
            24.0_sch * baseMultipler,
            Arg::center(Vec2{
                center.x + ResponsiveValue(text_x),
                center.y + ResponsiveValue(-0.311111111 * basePos.y + 78.6666)
            }),
            color
        );

        view_index++;
    }

    // +35 -> 0 -> +35
    // 220 -> 330 -> 220
}
