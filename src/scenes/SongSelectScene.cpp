#include "SongSelectScene.hpp"

template <typename T>
inline bool contains_vector(const std::vector<T>& vec, const T& value) {
    return std::find(vec.begin(), vec.end(), value) != vec.end();
}

void SongSelectScene::initialize()
{
    m_settingsOpen = false;
    m_settingsProgress = 0.0;
    m_settingsSelection = 0;

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
            additional_labels,
            object.contains(U"chart_creator_easy") ? object[U"chart_creator_easy"].getString() : U"",
            object.contains(U"chart_creator_normal") ? object[U"chart_creator_normal"].getString() : U"",
            object.contains(U"chart_creator_expert") ? object[U"chart_creator_expert"].getString() : U""
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

    for (const auto& [song_id, song_data] : m_songRegistry)
    {
        TextReader meta_reader{ CombinePath(song_data.songFilePath, U"meta") };
        String audio_extension;
        String jacket_extension;
        if (meta_reader && meta_reader.readLine(audio_extension) && meta_reader.readLine(jacket_extension))
        {
            const String jacket_path = CombinePath(song_data.songFilePath, U"jacket." + jacket_extension);
            if (FileSystem::Exists(jacket_path))
            {
                Texture jacket{ jacket_path };
                if (not jacket.isEmpty())
                {
                    m_jacketTextures.emplace(song_id, std::move(jacket));
                }
            }
        }
    }

    auto& data = getData();

    if (data.cache.contains(U"settings_note_speed")) {
        data.settings.noteSpeed = data.cache[U"settings_note_speed"].get<float>();
    }
    if (data.cache.contains(U"settings_offset")) {
        data.settings.offset = data.cache[U"settings_offset"].get<int32>();
    }
    data.cache[U"settings_note_speed"] = data.settings.noteSpeed;
    data.cache[U"settings_offset"] = data.settings.offset;

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
    
}

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
    const bool settings_was_open = m_settingsOpen;
    updateSettings();
    if (settings_was_open || m_settingsOpen) return;

    updateSongSelect();
}

void SongSelectScene::updateSettings()
{
    auto& data = getData();
    if (!m_settingsOpen) {
        if (KeyEscape.down() || KeyZ.down() || KeyX.down()) {
            m_settingsOpen = true;
        }
    }
    else if (KeyJ.down() || KeyEscape.down() || KeyZ.down() || KeyX.down()) {
        m_settingsOpen = false;
    }
    else if (KeyLeft.down() || KeyA.down()) {
        m_settingsSelection = std::max(0, m_settingsSelection - 1);
    }
    else if (KeyRight.down() || KeyD.down()) {
        m_settingsSelection = std::min(1, m_settingsSelection + 1);
    }
    else if (KeyUp.down() || KeyW.down()) {
        if (m_settingsSelection == 0) {
            data.settings.noteSpeed = std::min(10.0f, data.settings.noteSpeed + 0.1f);
        }
        else {
            data.settings.offset = std::min<int32>(200, data.settings.offset + 1);
        }
    }
    else if (KeyDown.down() || KeyS.down()) {
        if (m_settingsSelection == 0) {
            data.settings.noteSpeed = std::max(1.0f, data.settings.noteSpeed - 0.1f);
        }
        else {
            data.settings.offset = std::max<int32>(-200, data.settings.offset - 1);
        }
    }

    const double target = m_settingsOpen ? 1.0 : 0.0;
    const double progress_step = std::min(1.0, static_cast<double>(ResponsiveValue(0, 0.55)));
    m_settingsProgress += (target - m_settingsProgress) * progress_step;
    data.cache[U"settings_note_speed"] = data.settings.noteSpeed;
    data.cache[U"settings_offset"] = data.settings.offset;
}

void SongSelectScene::updateSongSelect()
{
    auto& data = getData();
    if (KeyJ.down())
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
    Rect{
        0, Scene::Height() - 25_sch,
        Scene::Width(), 25_sch
    }.draw(Palette::White);
    m_bodyFont(U"←→：楽曲選択　↑↓：難易度選択　①：戻る　②：決定　+/-：設定").draw(12.0_sch, Arg::bottomLeft(Vec2{ 13_sch, Scene::Height() - 4_sch }), Palette::Black);

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
        double baseMultipler = 2.0 / 3.0;
        if (render_index_i <= -1) {
            basePos = Vec2(-110 + 166.66666 * render_index_i, -90);
        }
        else if (-1 <= render_index_i && render_index_i < 0) {
            basePos = Vec2(-110 + 166.66666 * render_index_i, -165 - 75 * render_index_i);
            baseMultipler = - std::abs(render_index_i) / 3.0 + 1;
        }
        else if (0 <= render_index_i && render_index_i < 1) {
            basePos = Vec2(-110 + 240 * render_index_i, -165 + 75 * render_index_i);
            baseMultipler = - std::abs(render_index_i) / 3.0 + 1;
        }
        else {
            basePos = Vec2(-36.66666 + 166.66666 * render_index_i, -90);
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

        const Color outlineColor = Color{background_color.r * 0.8, background_color.g * 0.8, background_color.b * 0.8};
    
        const double card_x = center.x + ResponsiveValue(basePos.x);
        const double card_y = center.y + ResponsiveValue(basePos.y);
        const double card_width = ResponsiveValue(220 * baseMultipler);
        const double card_height = ResponsiveValue(330 * baseMultipler);

        RoundRect{
            card_x - 3_sch, card_y - 3_sch,
            card_width + 6_sch, card_height + 6_sch,
            8_sch
        }.draw(outlineColor);
        RoundRect{
            card_x, card_y,
            card_width, card_height,
            5_sch
        }.draw(background_color);

        const auto jacket_it = m_jacketTextures.find(song_data.id);
        if (jacket_it != m_jacketTextures.end())
        {
            const double jacket_size = 150_sch * baseMultipler;
            const double jacket_base_size = jacket_size + 12_sch * baseMultipler;
            RoundRect{
                card_x + card_width / 2 - jacket_base_size / 2,
                card_y + 95_sch * baseMultipler - jacket_base_size / 2,
                jacket_base_size,
                jacket_base_size,
                2_sch * baseMultipler
            }.draw(Palette::White);
            jacket_it->second.resized(jacket_size).draw(Arg::center(
                card_x + card_width / 2,
                card_y + 95_sch * baseMultipler
            ));
        }

        m_bodyFont(song_data.title).draw(
            24.0_sch * baseMultipler,
            Arg::center(card_x + card_width / 2, card_y + 205_sch * baseMultipler),
            color
        );
        m_bodyFont(song_data.artist).draw(
            18.0_sch * baseMultipler,
            Arg::center(card_x + card_width / 2, card_y + 242_sch * baseMultipler),
            color
        );

        view_index++;
    }

    // +35 -> 0 -> +35
    // 220 -> 330 -> 220

    if (m_settingsProgress > 0.0) {
        const double settings_scale = 1.0 + (1.35 - 1.0) * (1.0 - m_settingsProgress);
        const double settings_width = 640_sch * settings_scale;
        const double settings_height = 420_sch * settings_scale;
        const double settings_alpha = 1.0 * m_settingsProgress;
        const Color settings_backdrop{ U"#000000" };
        const Color settings_panel{ U"#464366" };
        const Color settings_accent{ U"#9792ff" };

        RectF{ 0_sch, 0_sch, Scene::Width(), Scene::Height() }.draw(ColorF{ settings_backdrop, 0.55 * m_settingsProgress });
        RoundRect{
            center.x - settings_width / 2,
            center.y - settings_height / 2,
            settings_width,
            settings_height,
            4_sch * settings_scale
        }.draw(ColorF{ settings_panel, settings_alpha });
        m_headingFont(U"設定").draw(30_sch * settings_scale, Arg::center(center.x, center.y - 138_sch * settings_scale), Color{ 222, 220, 255, settings_alpha * 255 });

        const double item_top = center.y - 65_sch * settings_scale;
        const double item_offset = 210_sch;
        const double value_y = item_top + 54_sch * settings_scale;
        // rgb(222, 222, 222)
        const Color description_color{ 222, 222, 222, settings_alpha * 255 };
        const ColorF selected_color{ settings_accent, settings_alpha };
        const bool isNoteSpeedInteger = !Format(data.settings.noteSpeed).includes(U".");
        const std::array<String, 2> item_names{ U"ノーツ速度", U"オフセット" };
        const std::array<String, 2> item_descriptions{ U"ノーツが流れる速度", U"判定位置の調整（ms）" };
        const std::array<String, 2> item_values{ Format(data.settings.noteSpeed) + (isNoteSpeedInteger ? U".0" : U""), Format(data.settings.offset) };

        for (int32 i = 0; i < 2; ++i) {
            const double item_x = center.x + (-item_offset + item_offset * i) * settings_scale;
            const bool selected = (m_settingsSelection == i);
            // rgb(222, 220, 255)
            const Color item_color{ 222, 220, 255, settings_alpha * 255 };
            const double value_width = 150_sch * settings_scale;

            m_bodyFont(item_names[i]).draw(22_sch * settings_scale, Arg::center(item_x, item_top), item_color);
            m_bodyFont(item_values[i]).draw(38_sch * settings_scale, Arg::center(item_x, value_y + 8_sch), item_color);
            m_bodyFont(item_descriptions[i]).draw(14_sch * settings_scale, Arg::center(item_x, value_y + 80_sch * settings_scale), description_color);

            if (selected) {
                const double underline_y = value_y + 30_sch * settings_scale;
                const double arrow_x = item_x;
                Line{ item_x - value_width / 2, underline_y, item_x + value_width / 2, underline_y }.draw(2_sch * settings_scale, selected_color);
                Triangle{
                    Vec2{ arrow_x, value_y - 30_sch * settings_scale },
                    Vec2{ arrow_x - 9_sch * settings_scale, value_y - 16_sch * settings_scale },
                    Vec2{ arrow_x + 9_sch * settings_scale, value_y - 16_sch * settings_scale }
                }.draw(selected_color);
                Triangle{
                    Vec2{ arrow_x, underline_y + 30_sch * settings_scale },
                    Vec2{ arrow_x - 9_sch * settings_scale, underline_y + 16_sch * settings_scale },
                    Vec2{ arrow_x + 9_sch * settings_scale, underline_y + 16_sch * settings_scale }
                }.draw(selected_color);
            }
        }
    }
}
