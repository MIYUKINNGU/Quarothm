#include "PlayScene.hpp"

#include "../ecs/RhythmComponents.hpp"

#include <numeric>

void PlayScene::initialize()
{
    m_state = State::Loading;
    m_elapsedMilliSec = 0.0;
    m_loadingVisualProgress = 0.0;

    const auto& song_data = getData().selectedSong.selectedSongData;
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
                m_loadingJacket = std::move(jacket);
            }
        }
    }

    m_future = std::async(
        std::launch::async,
        [this](){
            beginPlaying();
        }
    );
}

void PlayScene::updateFadeInImpl(double)
{
    if (m_state == State::Loading)
    {
        m_loadingVisualProgress = std::min(1.0, m_loadingVisualProgress + static_cast<double>(ResponsiveValue(0, 0.18)));
    }
}

void PlayScene::updateFadeOutImpl(double)
{
    if (m_state == State::Loading)
    {
        m_loadingVisualProgress = std::min(1.0, m_loadingVisualProgress + static_cast<double>(ResponsiveValue(0, 0.18)));
    }
}

void PlayScene::beginPlaying()
{
    auto &data = getData();

    m_world.clear();
    // ここで譜面・音源などの同期ロード後に必要な初期化を行います。
    String songDir = data.selectedSong.selectedSongData.songFilePath;

    TextReader meta_data{ CombinePath(songDir, U"meta") };
    if (!meta_data) {
        ThrowError(U"Failed to load.", U"Failed to open `" + CombinePath(songDir, U"meta") + U"`.");
    }
    String audio_type;
    if (!meta_data.readLine(audio_type)) {
        ThrowError(U"Failed to load.", U"A meta file is empty.");
    }

    m_song_audio = Audio(CombinePath(songDir, U"song." + audio_type));
    if (m_song_audio.isEmpty()) {
        ThrowError(U"Failed to load.", U"Failed to open `" + CombinePath(songDir, U"song." + audio_type) + U"`.");
    }

    if (contains_vector<String>(data.selectedSong.selectedSongData.additionalLabels, U"use_text")) {
        use_text = true;
        if (!FileSystem::Exists(CombinePath(songDir, U"text_data.json"))) {
            ThrowError(U"Failed to load.", U"Failed to open `" + CombinePath(songDir, U"text_data.json") + U"`.");
        }
        text_json_data = JSON::Load(CombinePath(songDir, U"text_data.json"));
    }

    TextReader chart_reader{ CombinePath(songDir, Format(static_cast<int32>(data.selectedSong.selectedDifficulty)) + U".sus") };

    String op;
    std::unordered_map<String, double> bpm_list;
    std::unordered_map<int32, String> bpm_changer;
    std::unordered_map<int32, int32> beats;
    //                measure                      lane                 species
    std::unordered_map<int32, std::unordered_map<char32_t, std::vector<NoteType>>> susData;
    int32 measure_max = 0;
    DebugLog(U"Ready to read and convert a .sus file.");

    // こういう特殊なファイル形式のデータ読み込みも別でクラスを用意して実装したほうが良い
    // (めんどいので端折ってるけどそっちのほうがバグもできにくいので絶対そうするべき)
    while (chart_reader.readLine(op)) {
        String trimmed_op = op.trim();
        DebugLog(U"Reading a line currently: " + trimmed_op);

        if (trimmed_op.starts_with(U"#ATR")) continue; // substr(4,2)と干渉する可能性がある
        if (trimmed_op.starts_with(U"#WAVEOFFSET")) {
            DebugLog(U"Loading a #WAVEOFFSET");
            m_chartData.chartOffset = - 1000 * Parse<double>(trimmed_op.substr(12));
            DebugLog(U"Loaded a #WAVEOFFSET");
            continue;
        }
        if (trimmed_op.starts_with(U"#REQUEST \"ticks_per_beat")) continue;
        if (trimmed_op.starts_with(U"#BPM")) {
            DebugLog(U"Loading a BPM Defination");
            String nonspaced_trimmed_op = trimmed_op.replaced(U" ", U"").replaced(U"\t", U"");
            bpm_list[nonspaced_trimmed_op.substr(4, 2)] = Parse<double>(nonspaced_trimmed_op.split(':')[1]);
            DebugLog(U"Loaded a BPM Defination");
            continue;
        }
        if (trimmed_op.size() > 4 && trimmed_op.substr(4, 2) == U"08") {
            DebugLog(U"Loading a BPM changer");
            String nonspaced_trimmed_op = trimmed_op.replaced(U" ", U"").replaced(U"\t", U"");
            bpm_changer[ParseInt<int32>(nonspaced_trimmed_op.substr(1, 3), Arg::radix = 10)] = nonspaced_trimmed_op.split(':')[1];
            DebugLog(U"Loaded a BPM changer");
            continue;
        }
        if (trimmed_op.size() > 4 && trimmed_op.substr(4, 2) == U"02") {
            DebugLog(U"Loading a beats");
            String nonspaced_trimmed_op = trimmed_op.replaced(U" ", U"").replaced(U"\t", U"");
            beats[ParseInt<int32>(nonspaced_trimmed_op.substr(1, 3), Arg::radix = 10)] = Parse<double>(nonspaced_trimmed_op.split(':')[1]);
            DebugLog(U"Loaded a beats");
            continue;
        }
        if (trimmed_op.size() > 6 && std::regex_match(trimmed_op.substr(0,6).toUTF8(), std::regex(R"(#[0-9a-z]{5})"))) {
            DebugLog(U"Loading a chart data: " + trimmed_op);
            // 絶対ここ関数に分けるべきだった

            DebugLog(U"lane Id: " + trimmed_op.substr(5,1));
            DebugLog(U"Operation Species: " + trimmed_op.substr(4,1));
            char32_t op_species = trimmed_op[4];
            char32_t lane = trimmed_op[5];
            DebugLog(U"measure(String): " + trimmed_op.substr(1,3));
            int32 measure = ParseInt<int32>(trimmed_op.substr(1,3), Arg::radix = 10);
            DebugLog(U"initilized measure");
            String notes_data = trimmed_op.split(':')[1];
            std::vector<NoteType> notes(notes_data.size() / 2);
            std::unordered_map<char32_t, std::vector<NoteType>> lanes = {};
            size_t scaler = 1;
            if (susData.find(measure) != susData.end()) {
                lanes = susData[measure];
            }
            if (lanes.find(lane) != lanes.end()) {
                notes = lanes[lane];

                const size_t oldSize = notes.size();
                const size_t newSize = notes_data.size() / 2;

                if (newSize != oldSize) {
                    // 既存データと今回のデータを両方とも表現できる分解能へ拡張
                    const size_t mergedSize = std::lcm(oldSize, newSize);
                    std::vector<NoteType> expanded(mergedSize, NoteType::None);

                    for (size_t i = 0; i < oldSize; ++i) {
                        expanded[i * mergedSize / oldSize] = notes[i];
                    }

                    notes = std::move(expanded);
                }

                scaler = notes.size() / newSize;
            }
            // 英語でも書いてるけどこういう処理は全てconstの連想配列型を用意しておけばめちゃめちゃ短く書ける
            // ひとまずop_species == U'5'は5行程度で書けるようになるはず
            // めんどくさいので後で変えるかも(このコメント残ってたら変えてない)
            for (size_t i = 0; i < notes_data.size(); i+=2) {
                // notes declaration
                if (op_species == U'1') {
                    // notes_data[i+1] is an information about a note length.
                    // here, it isn't used.

                    // in pjsk, this is a normal tap-note.
                    if (notes_data[i] == U'1') {
                        notes[scaler * i / 2] = NoteType::NormalTap;
                    }
                    // similarly, this is a critical tap-note.
                    else if (notes_data[i] == U'2') {
                        notes[scaler * i / 2] = NoteType::DoubleTap;
                    }
                    // and, this is a normal trace-note.
                    else if (notes_data[i] == U'5') {
                        notes[scaler * i / 2] = NoteType::Drag;
                    }
                    // finaly, this is a critical trace-note.
                    else if (notes_data[i] == U'6') {
                        // Nothing to do.
                    }
                }
                // defination about Hold/Slide note
                else if (op_species == U'3') {
                    // char32_t channel = trimmed_op[6];
                    // when this use slide note, please use the operation above.
                    
                    // starting point of hold note
                    if (notes_data[i] == U'1') {
                        notes[scaler * i / 2] = NoteType::Hold;
                    }
                    // end point of hold note
                    else if (notes_data[i] == U'2') {
                        notes[scaler * i / 2] = NoteType::Hold;
                    }
                }
                // defination about Flick note
                else if (op_species == U'5') {
                    // I wanna combine these. too long. and the nest also is too deep.

                    // USE UNORDERED_MAP. AND THEN YOU CAN SHORTEN THIS CODE SIGNIFICANTLY.

                    // upward
                    if (notes_data[i] == U'1') {
                        switch (lane) {
                        case U'2':
                            if (notes[scaler * i / 2] == NoteType::DoubleTap)
                                notes[scaler * i / 2] = NoteType::TapFlickRight;
                            else notes[scaler * i / 2] = NoteType::FlickRight; // lengthが同じとは限らない...ごみすぎ
                            break;
                        case U'5':
                            if (notes[scaler * i / 2] == NoteType::DoubleTap)
                                notes[scaler * i / 2] = NoteType::TapFlickDown;
                            else notes[scaler * i / 2] = NoteType::FlickDown;
                            break;
                        case U'8':
                            if (notes[scaler * i / 2] == NoteType::DoubleTap)
                                notes[scaler * i / 2] = NoteType::TapFlickLeft;
                            else notes[scaler * i / 2] = NoteType::FlickLeft;
                            break;
                        case U'b':
                            if (notes[scaler * i / 2] == NoteType::DoubleTap)
                                notes[scaler * i / 2] = NoteType::TapFlickUp;
                            else notes[scaler * i / 2] = NoteType::FlickUp;
                            break;
                        }
                    }
                    // Left direction
                    else if (notes_data[i] == U'3') {
                        switch (lane) {
                        case U'2':
                            if (notes[scaler * i / 2] == NoteType::DoubleTap)
                                notes[scaler * i / 2] = NoteType::TapFlickDown;
                            else notes[scaler * i / 2] = NoteType::FlickDown;
                            break;
                        case U'5':
                            if (notes[scaler * i / 2] == NoteType::DoubleTap)
                                notes[scaler * i / 2] = NoteType::TapFlickLeft;
                            else notes[scaler * i / 2] = NoteType::FlickLeft;
                            break;
                        case U'8':
                            if (notes[scaler * i / 2] == NoteType::DoubleTap)
                                notes[scaler * i / 2] = NoteType::TapFlickUp;
                            else notes[scaler * i / 2] = NoteType::FlickUp;
                            break;
                        case U'b':
                            if (notes[scaler * i / 2] == NoteType::DoubleTap)
                                notes[scaler * i / 2] = NoteType::TapFlickRight;
                            else notes[scaler * i / 2] = NoteType::FlickRight;
                            break;
                        }
                    }
                    // Right direction
                    else if (notes_data[i] == U'4') {
                        switch (lane) {
                        case U'2':
                            if (notes[scaler * i / 2] == NoteType::DoubleTap)
                                notes[scaler * i / 2] = NoteType::TapFlickUp;
                            else notes[scaler * i / 2] = NoteType::FlickUp;
                            break;
                        case U'5':
                            if (notes[scaler * i / 2] == NoteType::DoubleTap)
                                notes[scaler * i / 2] = NoteType::TapFlickRight;
                            else notes[scaler * i / 2] = NoteType::FlickRight;
                            break;
                        case U'8':
                            if (notes[scaler * i / 2] == NoteType::DoubleTap)
                                notes[scaler * i / 2] = NoteType::TapFlickDown;
                            else notes[scaler * i / 2] = NoteType::FlickDown;
                            break;
                        case U'b':
                            if (notes[scaler * i / 2] == NoteType::DoubleTap)
                                notes[scaler * i / 2] = NoteType::TapFlickLeft;
                            else notes[scaler * i / 2] = NoteType::FlickLeft;
                            break;
                        }
                    }
                }
            }
            lanes[lane] = notes;
            susData[measure] = lanes;
            measure_max = std::max(measure, measure_max);
            DebugLog(U"Loaded a chart data: " + trimmed_op);
        }
    }
    DebugLog(U"Loaded .sus file.");
    DebugLog(U"A wave offset: " + Format(m_chartData.chartOffset));

    DebugLog(U"susData.size(): " + Format(susData.size()));
    for (auto& [measure, lanes] : susData) {
        DebugLog(U"measure: " + Format(measure));
        for (auto& [lane, values] : lanes) {
            DebugLog(U"  Lane ID: " + Format(lane));
            DebugLog(U"  values.size(): " + Format(values.size()));
            for (size_t _ = 0; _ < values.size(); _++) {
                DebugLog(U"    " + Format(_) + U": " + Format(static_cast<int32>(values[_])));
            }
        }
    }
    // lane: 1 4 7 a
    double current_bpm = 0.0;
    double current_time = 0.0;
    double current_beats = 0.0;
    bool holding_note_left = false;
    bool holding_note_up = false;
    bool holding_note_right = false;
    bool holding_note_down = false;
    NoteComponent* starting_note_left;
    NoteComponent* starting_note_up;
    NoteComponent* starting_note_right;
    NoteComponent* starting_note_down;
    // こういうのは上手くポインタをやりくりすることで同じ部分を一つにまとめれたりする
    // 若干頭使う必要あるのであとで(このコメント残ってたら変えてない)
    for (int32 i = 0; i <= measure_max; i++) {
        if (bpm_changer.find(i) != bpm_changer.end()) {
            current_bpm = bpm_list[bpm_changer[i]];
        }
        if (beats.find(i) != beats.end()) {
            current_beats = beats[i];
        }
        if (susData.find(i) == susData.end()) {
            current_time += 60000.0 * current_beats / current_bpm;
            continue;
        }

        if (susData[i].find(U'2') != susData[i].end()) {
            double divider = 60000.0 * current_beats / susData[i][U'2'].size() / current_bpm;
            for (size_t j = 0; j < susData[i][U'2'].size(); j++) {
                NoteType note_type = susData[i][U'2'][j];
                DebugLog(U"measure " + Format(i) + U" lane2 note_type: " + Format(static_cast<int32>(note_type)) + U", time: " + Format(current_time + divider * j) + U", divider: " + Format(divider));

                if (note_type == NoteType::Hold && holding_note_left) {
                    holding_note_left = false;

                    starting_note_left->holdLength = current_time + divider * j - starting_note_left->hitTimeMilliSec;
                }
                else if (note_type == NoteType::Hold) {
                    auto note_component = std::make_unique<NoteComponent>(
                        current_time + divider * j,
                        0,
                        note_type
                    );
                    
                    NoteComponent* ptr = note_component.get();
                    starting_note_left = ptr;
                    holding_note_left = true;

                    noteStorage.push_back(std::move(note_component));
                    m_chartData.laneLeft.push_back(ptr);
                }
                else if (note_type != NoteType::None) {
                    auto note_component = std::make_unique<NoteComponent>(
                        current_time + divider * j,
                        0,
                        note_type
                    );
                    NoteComponent* ptr = note_component.get();

                    noteStorage.push_back(std::move(note_component));
                    m_chartData.laneLeft.push_back(ptr);
                }
            }
        }
        if (susData[i].find(U'5') != susData[i].end()) {
            double divider = 60000.0 * current_beats / susData[i][U'5'].size() / current_bpm;
            for (size_t j = 0; j < susData[i][U'5'].size(); j++) {
                NoteType note_type = susData[i][U'5'][j];
                DebugLog(U"measure " + Format(i) + U" lane5 note_type: " + Format(static_cast<int32>(note_type)) + U", time: " + Format(current_time + divider * j) + U", divider: " + Format(divider));

                if (note_type == NoteType::Hold && holding_note_up) {
                    holding_note_up = false;

                    starting_note_up->holdLength = current_time + divider * j - starting_note_up->hitTimeMilliSec;
                }
                else if (note_type == NoteType::Hold) {
                    holding_note_up = true;
                    auto note_component = std::make_unique<NoteComponent>(
                        current_time + divider * j,
                        2,
                        note_type
                    );
                    NoteComponent* ptr = note_component.get();
                    starting_note_up = ptr;

                    noteStorage.push_back(std::move(note_component));
                    m_chartData.laneUp.push_back(ptr);
                }
                else if (note_type != NoteType::None) {
                    auto note_component = std::make_unique<NoteComponent>(
                        current_time + divider * j,
                        2,
                        note_type
                    );
                    NoteComponent* ptr = note_component.get();

                    noteStorage.push_back(std::move(note_component));
                    m_chartData.laneUp.push_back(ptr);
                }
            }
        }
        if (susData[i].find(U'8') != susData[i].end()) {
            double divider = 60000.0 * current_beats / susData[i][U'8'].size() / current_bpm;
            for (size_t j = 0; j < susData[i][U'8'].size(); j++) {
                NoteType note_type = susData[i][U'8'][j];
                DebugLog(U"measure " + Format(i) + U" lane8 note_type: " + Format(static_cast<int32>(note_type)) + U", time: " + Format(current_time + divider * j) + U", divider: " + Format(divider));

                if (note_type == NoteType::Hold && holding_note_right) {
                    holding_note_right = false;

                    starting_note_right->holdLength = current_time + divider * j - starting_note_right->hitTimeMilliSec;
                }
                else if (note_type == NoteType::Hold) {
                    holding_note_right = true;
                    auto note_component = std::make_unique<NoteComponent>(
                        current_time + divider * j,
                        1,
                        note_type
                    );
                    NoteComponent* ptr = note_component.get();
                    starting_note_right = ptr;

                    noteStorage.push_back(std::move(note_component));
                    m_chartData.laneRight.push_back(ptr);
                }
                else if (note_type != NoteType::None) {
                    auto note_component = std::make_unique<NoteComponent>(
                        current_time + divider * j,
                        1,
                        note_type
                    );
                    NoteComponent* ptr = note_component.get();

                    noteStorage.push_back(std::move(note_component));
                    m_chartData.laneRight.push_back(ptr);
                }
            }
        }
        if (susData[i].find(U'b') != susData[i].end()) {
            double divider = 60000.0 * current_beats / susData[i][U'b'].size() / current_bpm;
            for (size_t j = 0; j < susData[i][U'b'].size(); j++) {
                NoteType note_type = susData[i][U'b'][j];
                DebugLog(U"measure " + Format(i) + U" laneb note_type: " + Format(static_cast<int32>(note_type)) + U", time: " + Format(current_time + divider * j) + U", divider: " + Format(divider));

                if (note_type == NoteType::Hold && holding_note_down) {
                    holding_note_down = false;

                    starting_note_down->holdLength = current_time + divider * j - starting_note_down->hitTimeMilliSec;
                }
                else if (note_type == NoteType::Hold) {
                    holding_note_down = true;
                    auto note_component = std::make_unique<NoteComponent>(
                        current_time + divider * j,
                        3,
                        note_type
                    );
                    NoteComponent* ptr = note_component.get();
                    starting_note_down = ptr;

                    noteStorage.push_back(std::move(note_component));
                    m_chartData.laneDown.push_back(ptr);
                }
                else if (note_type != NoteType::None) {
                    auto note_component = std::make_unique<NoteComponent>(
                        current_time + divider * j,
                        3,
                        note_type
                    );
                    NoteComponent* ptr = note_component.get();

                    noteStorage.push_back(std::move(note_component));
                    m_chartData.laneDown.push_back(ptr);
                }
            }
        }

        current_time += 60000.0 * current_beats / current_bpm;
    }

    // こんくらいの行数であれば工夫するより愚直にこうしたほうが早い
    // 別に可読性に大きく影響与えないのでね
    std::sort(
        m_chartData.laneDown.begin(),
        m_chartData.laneDown.end(),
        [](const NoteComponent* a, const NoteComponent* b) {
            return a->hitTimeMilliSec < b->hitTimeMilliSec;
        }
    );
    std::sort(
        m_chartData.laneUp.begin(),
        m_chartData.laneUp.end(),
        [](const NoteComponent* a, const NoteComponent* b) {
            return a->hitTimeMilliSec < b->hitTimeMilliSec;
        }
    );
    std::sort(
        m_chartData.laneLeft.begin(),
        m_chartData.laneLeft.end(),
        [](const NoteComponent* a, const NoteComponent* b) {
            return a->hitTimeMilliSec < b->hitTimeMilliSec;
        }
    );
    std::sort(
        m_chartData.laneRight.begin(),
        m_chartData.laneRight.end(),
        [](const NoteComponent* a, const NoteComponent* b) {
            return a->hitTimeMilliSec < b->hitTimeMilliSec;
        }
    );

    DebugLog(U"laneLeft.size(): " + Format(m_chartData.laneLeft.size()));
    DebugLog(U"laneUp.size(): " + Format(m_chartData.laneUp.size()));
    DebugLog(U"laneRight.size(): " + Format(m_chartData.laneRight.size()));
    DebugLog(U"laneDown.size(): " + Format(m_chartData.laneDown.size()));

    // 多分noteStorageからやればfor一つで済むよね
    // めんどくさいのでやっぱり後で(このコメント残ってたら変更してない)
    for (size_t i = 0; i < m_chartData.laneLeft.size(); i++) {
        NoteComponent* note_component = m_chartData.laneLeft[i];

        const Entity note = m_world.createEntity();
        m_world.addComponent<NoteComponent*>(note, note_component);

        double distance = (note_component->hitTimeMilliSec + m_chartData.chartOffset + data.settings.offset) * data.settings.noteSpeed * 0.1_sch;
        
        Point center = Scene::Center();

        m_world.addComponent<TransformComponent>(note, TransformComponent{ Vec2{ center.x - 40_sch - distance, center.y } });
    }
    for (size_t i = 0; i < m_chartData.laneUp.size(); i++) {
        NoteComponent* note_component = m_chartData.laneUp[i];

        const Entity note = m_world.createEntity();
        m_world.addComponent<NoteComponent*>(note, note_component);

        double distance = (note_component->hitTimeMilliSec + m_chartData.chartOffset + data.settings.offset) * data.settings.noteSpeed * 0.1_sch;
        
        Point center = Scene::Center();

        m_world.addComponent<TransformComponent>(note, TransformComponent{ Vec2{ center.x, center.y - 40_sch - distance } });
    }
    for (size_t i = 0; i < m_chartData.laneRight.size(); i++) {
        NoteComponent* note_component = m_chartData.laneRight[i];

        const Entity note = m_world.createEntity();
        m_world.addComponent<NoteComponent*>(note, note_component);

        double distance = (note_component->hitTimeMilliSec + m_chartData.chartOffset + data.settings.offset) * data.settings.noteSpeed * 0.1_sch;
        
        Point center = Scene::Center();

        m_world.addComponent<TransformComponent>(note, TransformComponent{ Vec2{ center.x + 40_sch + distance, center.y } });
    }
    for (size_t i = 0; i < m_chartData.laneDown.size(); i++) {
        NoteComponent* note_component = m_chartData.laneDown[i];

        const Entity note = m_world.createEntity();
        m_world.addComponent<NoteComponent*>(note, note_component);

        double distance = (note_component->hitTimeMilliSec + m_chartData.chartOffset + data.settings.offset) * data.settings.noteSpeed * 0.1_sch;
        
        Point center = Scene::Center();

        m_world.addComponent<TransformComponent>(note, TransformComponent{ Vec2{ center.x, center.y + 40_sch + distance } }); // こういうところ変える
    }

    // おお。だいぶ頭柔らかくなって調子取り戻してきてんじゃねえの
    std::pair<double, int32> m_value(100000000000000.0, 0);
    if (0 < m_chartData.laneLeft.size()) {
        std::pair<double, int32> ht = std::pair(m_chartData.laneLeft[0]->hitTimeMilliSec, 0);
        m_value = std::min(m_value, ht, [](const auto a, const auto b) {return a.first < b.first;});
    }
    if (0 < m_chartData.laneRight.size()) {
        std::pair<double, int32> ht = std::pair(m_chartData.laneRight[0]->hitTimeMilliSec, 1);
        m_value = std::min(m_value, ht, [](const auto a, const auto b) {return a.first < b.first;});
    }
    if (0 < m_chartData.laneUp.size()) {
        std::pair<double, int32> ht = std::pair(m_chartData.laneUp[0]->hitTimeMilliSec, 2);
        m_value = std::min(m_value, ht, [](const auto a, const auto b) {return a.first < b.first;});
    }
    if (0 < m_chartData.laneDown.size()) {
        std::pair<double, int32> ht = std::pair(m_chartData.laneDown[0]->hitTimeMilliSec, 3);
        m_value = std::min(m_value, ht, [](const auto a, const auto b) {return a.first < b.first;});
    }
    m_currentLane = m_value.second;
    m_renderRot = rot_map.at(m_currentLane);

    maxNotesCount = m_chartData.laneDown.size() + m_chartData.laneUp.size() + m_chartData.laneLeft.size() + m_chartData.laneRight.size();

    data.presentPlayData = ScoreRecorder{};

    m_state = State::Playing;

    m_song_audio.play();
}

size_t& PlayScene::getCurrentLaneReadingIdx() {
    switch (m_currentLane) {
    case 0:
        return reading_idx_left;
    case 1:
        return reading_idx_right;
    case 2:
        return reading_idx_up;
    case 3:
        return reading_idx_down;
    }

    ThrowError(U"Passed an undefined lane ID.", U"At PlayScene::getCurrentLaneReadingIdx().");
}

std::vector<NoteComponent*>& PlayScene::getCurrentLane() {
    switch (m_currentLane) {
    case 0:
        return m_chartData.laneLeft;
    case 1:
        return m_chartData.laneRight;
    case 2:
        return m_chartData.laneUp;
    case 3:
        return m_chartData.laneDown;
    }

    ThrowError(U"Passed an undefined lane ID.", U"At PlayScene::getCurrentLane().");
}

JudgementType PlayScene::judgeNote(
    NoteComponent* note_component,
    double perfect_plus_threshold,
    double perfect_threshold,
    double good_threshold,
    double miss_threshold
) {
    double diff = std::abs(getNoteDiff(note_component));
    if (diff < perfect_plus_threshold) return JudgementType::PerfectPlus;
    if (diff < perfect_threshold) return JudgementType::Perfect;
    if (diff < good_threshold) return JudgementType::Good;
    if (diff < miss_threshold) return JudgementType::Miss;
    if (-getNoteDiff(note_component) > miss_threshold) return JudgementType::Miss;
    return JudgementType::None;
}

bool PlayScene::isNoteLate(NoteComponent* note_component) const {
    return getNoteDiff(note_component) < 0;
}

// Late: -
// Early: +
double PlayScene::getNoteDiff(NoteComponent* note_component) const {
    return note_component->hitTimeMilliSec - m_elapsedMilliSec + m_chartData.chartOffset + getData().settings.offset;
}

void PlayScene::processNoteMiss(NoteComponent* note_component, size_t& index) {
    m_recentJudgement = JudgementType::Miss;
    processNormalJudged(note_component, index);
    getData().presentPlayData.maxCombo = std::max(getData().presentPlayData.maxCombo, combo);
    combo = 0;
}

void PlayScene::processNormalJudged(NoteComponent* note_component, size_t& index) {
    note_component->judged = true;
    m_showJudgeTime = Scene::Time();
    index++;
    m_renderJudgeSize = 0;
    m_judgeFrameCount = Scene::FrameCount();
    increaseJudgeCount();
}

void PlayScene::processJudging(NoteComponent* note_component, size_t& index) {
    m_recentJudgement = judgeNote(note_component);
    if (m_recentJudgement != JudgementType::None) {
        processNormalJudged(note_component, index);
    }

    if (m_recentJudgement == JudgementType::Miss) {
        combo = 0;
    }
    else if (m_recentJudgement != JudgementType::None) {
        combo++;
    }
}

void PlayScene::increaseJudgeCount() {
    auto& data = getData();
    switch (m_recentJudgement) {
    case JudgementType::Miss:
        data.presentPlayData.miss++;
        break;
    case JudgementType::Good:
        data.presentPlayData.good++;
        break;
    case JudgementType::PerfectPlus:
        data.presentPlayData.perfectplus++;
        break;
    case JudgementType::Perfect:
        data.presentPlayData.perfect++;
        break;
    }
}

void PlayScene::updateImpl()
{
    auto& data = getData();

    switch (m_state)
    {
    case State::Loading:
        break;

    case State::Playing: {
        m_elapsedMilliSec = m_song_audio.posSec() * 1000;
        m_world.each<TransformComponent>([this, data](const Entity note, TransformComponent& transform)
        {
            NoteComponent* note_component = *m_world.getComponent<NoteComponent*>(note);

            double distance = getNoteDiff(note_component) * data.settings.noteSpeed * 0.1_sch;

            Point center = Scene::Center();

            switch (note_component->lane) {
            case 0:
                transform.position = Vec2{ center.x - 40_sch - distance, center.y };
                break;
            case 2:
                transform.position = Vec2{ center.x, center.y - 40_sch - distance };
                break;
            case 1:
                transform.position = Vec2{ center.x + 40_sch + distance, center.y };
                break;
            case 3:
                transform.position = Vec2{ center.x, center.y + 40_sch + distance };
                break;
            }
        });

        std::vector<NoteComponent*>& current_lane = getCurrentLane();
        // DebugLog(U"m_currentLane: " + Format(m_currentLane));
        // DebugLog(U"reading_idx_left: " + Format(reading_idx_left));
        // DebugLog(U"reading_idx_right: " + Format(reading_idx_right));
        // DebugLog(U"reading_idx_up: " + Format(reading_idx_up));
        // DebugLog(U"reading_idx_down: " + Format(reading_idx_down));
        const auto current_note_it = std::find_if(current_lane.begin(), current_lane.end(), [](NoteComponent* note_component) {
            return !note_component->judged && note_component->noteType != NoteType::Hold;
        });
        const auto hold_note_it = std::find_if(current_lane.begin(), current_lane.end(), [](NoteComponent* note_component) {
            return !note_component->judged && note_component->noteType == NoteType::Hold;
        });

        if (current_note_it != current_lane.end()) {
            NoteComponent* current_note = *current_note_it;
            size_t current_idx = std::distance(current_lane.begin(), current_note_it);
            if (KeyJ.down() || KeyK.down()) {
                if (current_note->noteType == NoteType::NormalTap) {
                    processJudging(current_note, current_idx);
                }
            }
            if (KeyJ.down() && KeyK.pressed() || KeyJ.pressed() && KeyK.down()) {
                if (current_note->noteType == NoteType::DoubleTap) {
                    processJudging(current_note, current_idx);
                }
            }
            if (KeyLeft.down() || KeyA.down()) {
                if (current_note->noteType == NoteType::FlickLeft) {
                    processJudging(current_note, current_idx);
                }

                m_currentLane = 0;
            }
            if (KeyRight.down() || KeyD.down()) {
                if (current_note->noteType == NoteType::FlickRight) {
                    processJudging(current_note, current_idx);
                }

                m_currentLane = 1;
            }
            if (KeyUp.down() || KeyW.down()) {
                if (current_note->noteType == NoteType::FlickUp) {
                    processJudging(current_note, current_idx);
                }

                m_currentLane = 2;
            }
            if (KeyDown.down() || KeyS.down()) {
                if (current_note->noteType == NoteType::FlickDown) {
                    processJudging(current_note, current_idx);
                }

                m_currentLane = 3;
            }
            if (KeyK.pressed() || KeyJ.pressed()) {
                if (current_note->noteType == NoteType::Drag) {
                    if (isNoteLate(current_note)) {
                        processJudging(current_note, current_idx);
                    }
                }
            }
        }

        if (hold_note_it != current_lane.end()) {
            NoteComponent* hold_note = *hold_note_it;
            if (KeyJ.down() || KeyK.down()) {
                JudgementType judge = judgeNote(hold_note);
                if (judge != JudgementType::Miss && judge != JudgementType::None) {
                    starting_point_tapped = true;
                }
            }
            if (!(KeyK.pressed() || KeyJ.pressed())) {
                starting_point_tapped = false;
            }
        }

        if (KeyLeft.down() || KeyA.down()) {
            m_currentLane = 0;
            m_frameCount = Scene::FrameCount();
        }
        if (KeyRight.down() || KeyD.down()) {
            m_currentLane = 1;
            m_frameCount = Scene::FrameCount();
        }
        if (KeyUp.down() || KeyW.down()) {
            m_currentLane = 2;
            m_frameCount = Scene::FrameCount();
        }
        if (KeyDown.down() || KeyS.down()) {
            m_currentLane = 3;
            m_frameCount = Scene::FrameCount();
        }

        const double timing_offset = m_chartData.chartOffset + data.settings.offset;
        const auto process_lane_notes = [this, timing_offset](std::vector<NoteComponent*>& lane) {
            const auto note_it = std::find_if(lane.begin(), lane.end(), [](NoteComponent* note_component) {
                return !note_component->judged && note_component->noteType != NoteType::Hold;
            });
            if (note_it != lane.end()) {
                NoteComponent* note_component = *note_it;
                size_t note_idx = std::distance(lane.begin(), note_it);
                if (m_elapsedMilliSec - note_component->hitTimeMilliSec - timing_offset > 150.0) {
                    processNoteMiss(note_component, note_idx);
                }
            }

            const auto hold_note_it = std::find_if(lane.begin(), lane.end(), [](NoteComponent* note_component) {
                return !note_component->judged && note_component->noteType == NoteType::Hold;
            });
            if (hold_note_it != lane.end()) {
                NoteComponent* hold_note = *hold_note_it;
                size_t hold_note_idx = std::distance(lane.begin(), hold_note_it);
                if (!starting_point_tapped && m_elapsedMilliSec - hold_note->hitTimeMilliSec - timing_offset > 150.0) {
                    processNoteMiss(hold_note, hold_note_idx);
                }
                else if (starting_point_tapped && m_elapsedMilliSec - hold_note->hitTimeMilliSec - hold_note->holdLength - timing_offset > 0.0) {
                    m_recentJudgement = JudgementType::PerfectPlus;
                    processNormalJudged(hold_note, hold_note_idx);
                    starting_point_tapped = false;
                }
            }
        };

        process_lane_notes(m_chartData.laneLeft);
        process_lane_notes(m_chartData.laneRight);
        process_lane_notes(m_chartData.laneUp);
        process_lane_notes(m_chartData.laneDown);

        double basePoint = 100000000.0 / maxNotesCount;
        data.presentPlayData.score = data.presentPlayData.perfectplus + ((data.presentPlayData.perfectplus + data.presentPlayData.perfect) + 0.3333 * data.presentPlayData.good) * basePoint;
        unsigned int notesCount = data.presentPlayData.perfectplus + data.presentPlayData.perfect + data.presentPlayData.good + data.presentPlayData.miss;
        if (notesCount != 0) data.presentPlayData.accuracy = 100.0 * (data.presentPlayData.perfectplus + data.presentPlayData.perfect / 1.25 + data.presentPlayData.good / 3.0) / notesCount;
        else data.presentPlayData.accuracy = 100.0;
        if (KeyR.down() || !m_song_audio.isPlaying())
        {
            if (data.presentPlayData.accuracy >= 70.0 && notesCount != 0) {
                data.presentPlayData.clear = ClearType::Cleared;
                if (maxNotesCount == data.presentPlayData.perfectplus) {
                    data.presentPlayData.clear = ClearType::MaxScore;
                }
                else if (maxNotesCount == data.presentPlayData.perfect + data.presentPlayData.perfectplus) {
                    data.presentPlayData.clear = ClearType::AllPerfect;
                }
                else if (maxNotesCount == combo) {
                    data.presentPlayData.clear = ClearType::FullCombo;
                }
            }
            else {
                data.presentPlayData.clear = ClearType::NonCleared;
            }
            data.presentPlayData.maxCombo = std::max(data.presentPlayData.maxCombo, combo);
            m_state = State::Finished;
        }

        // 1フレームに一回処理するもの
        if (m_beforeFrameCount != Scene::FrameCount()) {
            double target = rot_map.at(m_currentLane);
            if (static_cast<int32>((target-m_renderRot))
             >= static_cast<int32>(std::numbers::pi)) {
                target -= 2*std::numbers::pi;
            }
            if (static_cast<int32>(std::abs(m_renderRot-target))
             >= static_cast<int32>(std::numbers::pi)) {
                target += 2*std::numbers::pi;
            }
            m_renderRot += (m_renderRot - target) * (std::pow(0.3, ResponsiveValue(0, static_cast<double>(Scene::FrameCount()-m_frameCount))) - 1);
            if (m_renderRot < 0) {
                m_renderRot = std::fmod(m_renderRot, 2.0*std::numbers::pi) + 2.0*std::numbers::pi;
            }
            else {
                m_renderRot = std::fmod(m_renderRot, 2.0*std::numbers::pi);
            }

            // DebugLog(U"m_renderJudgeSize: " + Format(m_renderJudgeSize));
            m_renderJudgeSize += (m_renderJudgeSize - 1) * (std::pow(0.4, ResponsiveValue(0, static_cast<double>(Scene::FrameCount()-m_judgeFrameCount))) - 1);

            m_beforeFrameCount = Scene::FrameCount();
        }
        break;
    }
    case State::Finished:
        changeScene(SceneState::Result, 0.3s);
        break;
    }
}

void PlayScene::drawImpl() const
{
    if (m_state == State::Loading)
    {
        const auto& data = getData();
        const Point center = Scene::Center();
        const double jacket_start_size = 230_sch;
        const double jacket_end_size = 180_sch;
        const double jacket_size = jacket_start_size + (jacket_end_size - jacket_start_size) * m_loadingVisualProgress;
        const double jacket_base_padding = 16_sch;
        const double jacket_base_size = jacket_size + jacket_base_padding;
        const double jacket_center_y_offset = -45_sch;
        const double jacket_center_y = center.y + jacket_center_y_offset;
        const double text_fade_start = 0.55;
        const double text_alpha = std::clamp((m_loadingVisualProgress - text_fade_start) / (1.0 - text_fade_start), 0.0, 1.0);
        const double loading_section_height = Scene::Height() / 4.0;
        const String chart_creator = [&]() {
            switch (data.selectedSong.selectedDifficulty) {
            case Difficulty::Easy:
                return data.selectedSong.selectedSongData.chartCreatorEasy;
            case Difficulty::Normal:
                return data.selectedSong.selectedSongData.chartCreatorNormal;
            case Difficulty::Expert:
                return data.selectedSong.selectedSongData.chartCreatorExpert;
            }
            return String{};
        }();

        const double content_center_y = loading_section_height + loading_section_height;
        const double content_offset_y = content_center_y - center.y;
        const double image_center_y = jacket_center_y + content_offset_y;
        RectF{
            center.x - jacket_base_size / 2,
            image_center_y - jacket_base_size / 2,
            jacket_base_size,
            jacket_base_size
        }.draw(Palette::White);
        if (not m_loadingJacket.isEmpty())
        {
            m_loadingJacket.resized(jacket_size).draw(Arg::center(center.x, image_center_y));
        }

        m_font(data.selectedSong.selectedSongData.artist).draw(20_sch, Arg::center(center.x, image_center_y + jacket_size / 2 + 30_sch), ColorF{ Palette::White, text_alpha });
        m_font(U"譜面制作: " + chart_creator).draw(16_sch, Arg::center(center.x, image_center_y + jacket_size / 2 + 60_sch), ColorF{ Palette::White, text_alpha });
        return;
    }

    auto& data = getData();


    /*
    structure of text_json_data
    [
        {
            "show_time": number, // seconds
            "length": number, // seconds
            "fade_in": boolean, // fade in over 0.4 seconds when true
            "fade_out": boolean, // fade out over 0.4 seconds when true
            "objects": [ // draw in order from index 0
                {
                    "type": "text" | "image",
                    "data": string, // when "type" is "text", it's plain text, and when that's "image", it's relative path to an image data. (based on the game's current directory)
                    "position_type": "relative" | "absolute", // when this is "relative", use _sch value for position and size.
                    "pos": {
                        "x": number,
                        "y": number
                    },
                    "size": number, // text font size. For images, specify { "width": number, "height": number }.
                    "anchor_type": "righttop" | "centertop" | "lefttop" | "rightcenter" | "center" | ..., // naming rule: horizontal -> vertical. but "center", not "centercenter"
                    "base_position_type": "righttop" | "centertop" | "lefttop" | "rightcenter" | "center" | ..., // this rule is same as above
                },
                ...
            ]
        },
        ...
    ]
    */

    Point center = Scene::Center();

    if (use_text && text_json_data.isArray())
    {
        const auto getAnchorPosition = [](const String& anchor) {
            const double width = Scene::Width();
            const double height = Scene::Height();
            if (anchor == U"righttop") return Vec2{ width, 0.0 };
            if (anchor == U"centertop") return Vec2{ width / 2.0, 0.0 };
            if (anchor == U"lefttop") return Vec2{ 0.0, 0.0 };
            if (anchor == U"rightcenter") return Vec2{ width, height / 2.0 };
            if (anchor == U"leftcenter") return Vec2{ 0.0, height / 2.0 };
            if (anchor == U"rightbottom") return Vec2{ width, height };
            if (anchor == U"centerbottom") return Vec2{ width / 2.0, height };
            if (anchor == U"leftbottom") return Vec2{ 0.0, height };
            return Scene::CenterF();
        };

        for (const auto& [index, text_data] : text_json_data)
        {
            const double show_time = text_data[U"show_time"].get<double>();
            const double length = text_data[U"length"].get<double>();
            // Text timing follows the chart timeline, so apply both the chart
            // offset and the user offset (settings.offset is in milliseconds).
            const double current_time = (m_elapsedMilliSec - m_chartData.chartOffset - data.settings.offset) / 1000.0;
            if ((current_time < show_time) || (show_time + length < current_time))
            {
                continue;
            }

            constexpr double text_fade_duration = 0.4;
            const bool fade_in = text_data[U"fade_in"].getOr<bool>(false);
            const bool fade_out = text_data[U"fade_out"].getOr<bool>(false);
            double text_alpha = 1.0;
            if (fade_in)
            {
                text_alpha = Min(text_alpha, Clamp((current_time - show_time) / text_fade_duration, 0.0, 1.0));
            }
            if (fade_out)
            {
                text_alpha = Min(text_alpha, Clamp((show_time + length - current_time) / text_fade_duration, 0.0, 1.0));
            }
            const ColorF text_color{ Palette::White, text_alpha };

            for (const auto& [object_index, object] : text_data[U"objects"])
            {
                const String object_type = object[U"type"].getString();
                const String object_data = object[U"data"].getString();
                const String position_type = object[U"position_type"].getOr<String>(U"absolute");
                const auto& position_data = object[U"pos"];
                const String base_position_type = object[U"base_position_type"].getString();
                const Vec2 base_position = getAnchorPosition(base_position_type);
                const double x = position_data[U"x"].get<double>();
                const double y = position_data[U"y"].get<double>();
                const double position_x = (position_type == U"relative") ? static_cast<double>(ResponsiveValue(x)) : x;
                const double position_y = (position_type == U"relative") ? static_cast<double>(ResponsiveValue(y)) : y;
                const Vec2 position = base_position + Vec2{
                    position_x,
                    position_y
                };
                const String anchor = object[U"anchor_type"].getString();

                if (object_type == U"text")
                {
                    const double size = object[U"size"].getOr<double>(22.0);
                    const double font_size = (position_type == U"relative") ? static_cast<double>(ResponsiveValue(size)) : size;
                    if (anchor == U"righttop") m_font(object_data).draw(font_size, Arg::topRight(position), text_color);
                    else if (anchor == U"centertop") m_font(object_data).draw(font_size, Arg::topCenter(position), text_color);
                    else if (anchor == U"lefttop") m_font(object_data).draw(font_size, Arg::topLeft(position), text_color);
                    else if (anchor == U"rightcenter") m_font(object_data).draw(font_size, Arg::rightCenter(position), text_color);
                    else if (anchor == U"leftcenter") m_font(object_data).draw(font_size, Arg::leftCenter(position), text_color);
                    else if (anchor == U"rightbottom") m_font(object_data).draw(font_size, Arg::bottomRight(position), text_color);
                    else if (anchor == U"centerbottom") m_font(object_data).draw(font_size, Arg::bottomCenter(position), text_color);
                    else if (anchor == U"leftbottom") m_font(object_data).draw(font_size, Arg::bottomLeft(position), text_color);
                    else m_font(object_data).draw(font_size, Arg::center(position), text_color);
                }
                else if (object_type == U"image")
                {
                    auto image_it = m_textTextures.find(object_data);
                    if (image_it == m_textTextures.end())
                    {
                        image_it = m_textTextures.emplace(object_data, Texture{ object_data }).first;
                    }
                    if (not image_it->second.isEmpty())
                    {
                        const auto& size_data = object[U"size"];
                        const double width = size_data[U"width"].getOr<double>(image_it->second.width());
                        const double height = size_data[U"height"].getOr<double>(image_it->second.height());
                        const double image_width = (position_type == U"relative") ? static_cast<double>(ResponsiveValue(width, true)) : width;
                        const double image_height = (position_type == U"relative") ? static_cast<double>(ResponsiveValue(height)) : height;
                        const auto image = image_it->second.resized(image_width, image_height);
                        if (anchor == U"righttop") image.draw(Arg::topRight(position), text_color);
                        else if (anchor == U"centertop") image.draw(Arg::topCenter(position), text_color);
                        else if (anchor == U"lefttop") image.draw(Arg::topLeft(position), text_color);
                        else if (anchor == U"rightcenter") image.draw(Arg::rightCenter(position), text_color);
                        else if (anchor == U"leftcenter") image.draw(Arg::leftCenter(position), text_color);
                        else if (anchor == U"rightbottom") image.draw(Arg::bottomRight(position), text_color);
                        else if (anchor == U"centerbottom") image.draw(Arg::bottomCenter(position), text_color);
                        else if (anchor == U"leftbottom") image.draw(Arg::bottomLeft(position), text_color);
                        else image.draw(Arg::center(position), text_color);
                    }
                }
            }
        }
    }

    Line{
        center.x, 0,
        center.x, center.y - 40_sch
    }.draw(1_sch, Palette::Gray);
    Line{
        center.x, center.y + 40_sch,
        center.x, Scene::Height()
    }.draw(1_sch, Palette::Gray);
    Line{
        0, center.y,
        center.x - 40_sch, center.y
    }.draw(1_sch, Palette::Gray);
    Line{
        Scene::Width(), center.y,
        center.x + 40_sch, center.y
    }.draw(1_sch, Palette::Gray);

    // Rect{ center.x - 40_sch, center.y - 40_sch, 80_sch, 80_sch }.drawFrame(2_scw, Palette::Gray);
    // Left
    Line{
        center.x - 40_sch, center.y + 30_sch,
        center.x - 40_sch, center.y - 30_sch
    }.draw(3_sch, Palette::Gray);
    // Right
    Line{
        center.x + 40_sch, center.y + 30_sch,
        center.x + 40_sch, center.y - 30_sch
    }.draw(3_sch, Palette::Gray);
    // Top
    Line{
        center.x - 30_sch, center.y - 40_sch,
        center.x + 30_sch, center.y - 40_sch
    }.draw(3_sch, Palette::Gray);
    // Bottom
    Line{
        center.x - 30_sch, center.y + 40_sch,
        center.x + 30_sch, center.y + 40_sch
    }.draw(3_sch, Palette::Gray);

    // Current
    // rotation matrix
    // [ cos(theta) -sin(theta) ][x]
    // [ sin(theta)  sin(theta) ][y]
    Line{
        center.x + ((-40_sch)*std::cos(m_renderRot) - (30_sch)*std::sin(m_renderRot)),
        center.y + ((-40_sch)*std::sin(m_renderRot) + (30_sch)*std::cos(m_renderRot)),

        center.x + ((-40_sch)*std::cos(m_renderRot) - (-30_sch)*std::sin(m_renderRot)),
        center.y + ((-40_sch)*std::sin(m_renderRot) + (-30_sch)*std::cos(m_renderRot)),
    }.draw(KeyK.pressed() || KeyJ.pressed() ? 5_sch : 3_sch, Palette::White);

    std::vector<std::pair<Entity, std::reference_wrapper<const TransformComponent>>> notes;

    m_world.each<TransformComponent>(
        [&notes](const Entity note, const TransformComponent& transform)
        {
            notes.emplace_back(note, std::cref(transform));
        }
    );

    std::sort(notes.begin(), notes.end(),
        [this](const auto& a, const auto& b)
        {
            NoteComponent* note_a =
                *m_world.getComponent<NoteComponent*>(a.first);

            NoteComponent* note_b =
                *m_world.getComponent<NoteComponent*>(b.first);

            const bool a_is_hold = (note_a->noteType == NoteType::Hold);
            const bool b_is_hold = (note_b->noteType == NoteType::Hold);

            if (a_is_hold != b_is_hold)
            {
                return a_is_hold;
            }

            return note_a->hitTimeMilliSec > note_b->hitTimeMilliSec;
        }
    );

    for (const auto& [note, transform_ref] : notes)
    {
        const TransformComponent& transform = transform_ref.get();

        NoteComponent* note_component = *m_world.getComponent<NoteComponent*>(note);
        
        // auto& data = getData();
        // DebugLog(
        //     U"[" + Format(m_elapsedMilliSec) + U"] id [" + Format(note)
        //     + U"] hitTime(ms): " + Format(note_component->hitTimeMilliSec + m_chartData.chartOffset)
        //     + U", transform.position: " + Format(transform.position)
        //     + U", ETA(ms): " + Format(note_component->hitTimeMilliSec + m_chartData.chartOffset + data.settings.offset - m_elapsedMilliSec)
        //     + U", isJudged?: " + Format(note_component->judged));

        if (!note_component->judged) {
            switch (note_component->noteType) {
                case NoteType::None:
                    break;
                case NoteType::NormalTap:
                    Circle{ transform.position, 15_sch }.draw(Palette::White);
                    Circle{ transform.position, 15_sch }.drawFrame(2_sch, Palette::Black);
                    break;
                case NoteType::FlickLeft:
                    normal_left.resized(40_sch).draw(Arg::center(transform.position));
                    break;
                case NoteType::FlickRight:
                    normal_right.resized(40_sch).draw(Arg::center(transform.position));
                    break;
                case NoteType::FlickUp:
                    normal_up.resized(40_sch).draw(Arg::center(transform.position));
                    break;
                case NoteType::FlickDown:
                    normal_down.resized(40_sch).draw(Arg::center(transform.position));
                    break;
                case NoteType::Drag:
                    Circle{ transform.position, 8_sch }.draw(Palette::Yellow);
                    Circle{ transform.position, 8_sch }.drawFrame(1_sch, Palette::Cyan);
                    break;
                case NoteType::DoubleTap:
                    Circle{ transform.position, 15_sch }.draw(Palette::White);
                    Circle{ transform.position, 15_sch }.drawFrame(3_sch, Palette::Orange);
                    break;
                case NoteType::Hold: {

                    double distance = (getNoteDiff(note_component) + note_component->holdLength) * data.settings.noteSpeed * 0.1_sch;

                    Point center = Scene::Center();
                    Vec2 end_pos = transform.position;
                    Vec2 start_pos = transform.position;

                    switch (note_component->lane) {
                    case 0:
                        end_pos = Vec2{ center.x - 40_sch - distance, center.y };
                        if (isNoteLate(note_component)) start_pos = Vec2{ center.x - 40_sch, center.y };
                        break;
                    case 2:
                        end_pos = Vec2{ center.x, center.y - 40_sch - distance };
                        if (isNoteLate(note_component)) start_pos = Vec2{ center.x, center.y - 40_sch };
                        break;
                    case 1:
                        end_pos = Vec2{ center.x + 40_sch + distance, center.y };
                        if (isNoteLate(note_component)) start_pos = Vec2{ center.x + 40_sch, center.y };
                        break;
                    case 3:
                        end_pos = Vec2{ center.x, center.y + 40_sch + distance };
                        if (isNoteLate(note_component)) start_pos = Vec2{ center.x, center.y + 40_sch };
                        break;
                    }
                    Line{ start_pos, end_pos }.draw(10_sch, Palette::Cyan);
                    break;
                }
                default:
                    Circle{ transform.position, 15_sch }.draw(Palette::White);
                    break;
            }
        }
    }

    if (Time::GetSec() - m_showJudgeTime < 1.0) {
        switch (m_recentJudgement) {
            case JudgementType::PerfectPlus:
                m_font(U"Perfect+").draw(TextStyle::Outline(0.1_sch, Palette::White), 15_sch * m_renderJudgeSize, Arg::center(center - Vec2(0, 18_sch)), Palette::Pink);
                break;
            case JudgementType::Perfect:
                m_font(U"Perfect").draw(TextStyle::Outline(0.1_sch, Palette::White), 15_sch * m_renderJudgeSize, Arg::center(center - Vec2(0, 18_sch)), Palette::Purple);
                break;
            case JudgementType::Good:
                m_font(U"Good").draw(TextStyle::Outline(0.1_sch, Palette::White), 15_sch * m_renderJudgeSize, Arg::center(center - Vec2(0, 18_sch)), Palette::Orange);
                break;
            case JudgementType::Miss:
                m_font(U"Miss").draw(TextStyle::Outline(0.1_sch, Palette::White), 15_sch * m_renderJudgeSize, Arg::center(center - Vec2(0, 18_sch)), Palette::Red);
                break;
        }
    }
    if (combo >= 2) {
        m_font(Format(combo)).draw(12_sch, Arg::center(center + Vec2(0, 18_sch)), Palette::White);
    }
    m_font(U"{:09}"_fmt(data.presentPlayData.score)).draw(Arg::topRight(Vec2(Scene::Width() - 20, 20)), Palette::White);
    m_font(U"{:.2f}%"_fmt(data.presentPlayData.accuracy)).draw(Arg::topRight(Vec2(Scene::Width() - 20, 42)), Palette::White);
}
