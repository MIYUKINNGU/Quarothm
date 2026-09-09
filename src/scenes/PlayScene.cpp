#include "PlayScene.hpp"

#include "../ecs/RhythmComponents.hpp"

void PlayScene::initialize()
{
    m_state = State::Loading;
    m_elapsedMilliSec = 0.0;

    m_future = std::async(
        std::launch::async,
        [this](){
            beginPlaying();
        }
    );
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

    TextReader chart_reader{ CombinePath(songDir, Format(static_cast<int32>(data.selectedSong.selectedDifficulty)) + U".sus") };

    String op;
    std::unordered_map<int32, double> bpm_list;
    std::unordered_map<int32, int32> bpm_changer;
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
            bpm_list[ParseInt<int32>(nonspaced_trimmed_op.substr(4, 2))] = Parse<double>(nonspaced_trimmed_op.split(':')[1]);
            DebugLog(U"Loaded a BPM Defination");
            continue;
        }
        if (trimmed_op.size() > 4 && trimmed_op.substr(4, 2) == U"08") {
            DebugLog(U"Loading a BPM changer");
            String nonspaced_trimmed_op = trimmed_op.replaced(U" ", U"").replaced(U"\t", U"");
            bpm_changer[ParseInt<int32>(nonspaced_trimmed_op.substr(1, 3))] = ParseInt<int32>(nonspaced_trimmed_op.split(':')[1]);
            DebugLog(U"Loaded a BPM changer");
            continue;
        }
        if (trimmed_op.size() > 4 && trimmed_op.substr(4, 2) == U"02") {
            DebugLog(U"Loading a beats");
            String nonspaced_trimmed_op = trimmed_op.replaced(U" ", U"").replaced(U"\t", U"");
            beats[ParseInt<int32>(nonspaced_trimmed_op.substr(1, 3))] = Parse<double>(nonspaced_trimmed_op.split(':')[1]);
            DebugLog(U"Loaded a beats");
            continue;
        }
        if (trimmed_op.size() > 6 && std::regex_match(trimmed_op.substr(0,6).toUTF8(), std::regex(R"(#[0-9a-z]{5})"))) {
            DebugLog(U"Loading a chart data: " + trimmed_op);
            // 絶対ここ関数に分けるべきだった

            DebugLog(U"lane Id: " + trimmed_op.substr(5,1));
            char32_t op_species = trimmed_op[4];
            char32_t lane = trimmed_op[5];
            int32 measure = ParseInt<int32>(trimmed_op.substr(1,3));
            String notes_data = trimmed_op.split(':')[1];
            std::vector<NoteType> notes(notes_data.size() / 2);
            std::unordered_map<char32_t, std::vector<NoteType>> lanes = {};
            size_t scaler = 1;
            if (susData.find(measure) != susData.end()) {
                lanes = susData[measure];
            }
            if (lanes.find(lane) != lanes.end()) {
                notes = lanes[lane];
                scaler = 2 * notes.size() / notes_data.size();
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
        if (susData.find(i) == susData.end()) continue;

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

    m_state = State::Playing;

    m_song_audio.play();
}

void PlayScene::updateImpl()
{
    auto& data = getData();

    switch (m_state)
    {
    case State::Loading:
        // 非同期で状態更新もしてるのですることがない
        break;

    case State::Playing:
        m_elapsedMilliSec = m_song_audio.posSec() * 1000;
        // NoteMoveSystem相当: Noteを持つEntityの位置を一括更新します。
        m_world.each<TransformComponent>([this, data](const Entity note, TransformComponent& transform)
        {
            NoteComponent* note_component = *m_world.getComponent<NoteComponent*>(note);

            double distance = (note_component->hitTimeMilliSec + m_chartData.chartOffset + data.settings.offset - m_elapsedMilliSec) * data.settings.noteSpeed * 0.1_sch;
            DebugLog(U"[" + Format(m_elapsedMilliSec) + U"] id [" + Format(note) + U"] hitTime(ms): " + Format(note_component->hitTimeMilliSec + m_chartData.chartOffset) + U", distance: " + Format(distance) + U", ETA(ms): " + Format(note_component->hitTimeMilliSec + m_chartData.chartOffset + data.settings.offset - m_elapsedMilliSec));

            Point center = Scene::Center();

            switch (note_component->lane) {
            case 0:
                transform.position = Vec2{ center.x - 40_sch - distance, center.y };
                break;
            case 1:
                transform.position = Vec2{ center.x, center.y - 40_sch - distance };
                break;
            case 2:
                transform.position = Vec2{ center.x + 40_sch + distance, center.y };
                break;
            case 3:
                transform.position = Vec2{ center.x, center.y + 40_sch + distance };
                break;
            }
        });

        if (KeyLeft.down() || KeyA.down()) {
            m_currentLane = 0;
        }
        if (KeyRight.down() || KeyD.down()) {
            m_currentLane = 1;
        }
        if (KeyUp.down() || KeyW.down()) {
            m_currentLane = 2;
        }
        if (KeyDown.down() || KeyS.down()) {
            m_currentLane = 3;
        }

        if (KeyR.down() || !m_song_audio.isPlaying())
        {
            getData().presentPlayData.score = 987654;
            getData().presentPlayData.maxCombo = 123;
            m_state = State::Finished;
        }
        break;

    case State::Finished:
        changeScene(SceneState::Result, 0.25s);
        break;
    }
}

void PlayScene::drawImpl() const
{
    if (m_state == State::Loading)
    {
        m_font(U"NOW LOADING...").drawAt(Scene::Center(), Palette::White);
        return;
    }

    Point center = Scene::Center();

    // Rect{ center.x - 40_sch, center.y - 40_sch, 80_sch, 80_sch }.drawFrame(2_scw, Palette::Gray);
    // Left
    Line{
        center.x - 40_sch, center.y + 30_sch,
        center.x - 40_sch, center.y - 30_sch
    }.draw(3_sch, m_currentLane == 0 ? Palette::White : Palette::Gray);
    // Right
    Line{
        center.x + 40_sch, center.y + 30_sch,
        center.x + 40_sch, center.y - 30_sch
    }.draw(3_sch, m_currentLane == 1 ? Palette::White : Palette::Gray);
    // Top
    Line{
        center.x - 30_sch, center.y - 40_sch,
        center.x + 30_sch, center.y - 40_sch
    }.draw(3_sch, m_currentLane == 2 ? Palette::White : Palette::Gray);
    // Bottom
    Line{
        center.x - 30_sch, center.y + 40_sch,
        center.x + 30_sch, center.y + 40_sch
    }.draw(3_sch, m_currentLane == 3 ? Palette::White : Palette::Gray);

    Line{
        center.x, 0,
        center.x, Scene::Height()
    }.draw(1_sch, Palette::Gray);
    Line{
        0, center.y,
        Scene::Width(), center.y
    }.draw(1_sch, Palette::Gray);
    m_world.each<TransformComponent>([](const Entity, const TransformComponent& transform)
    {
        Circle{ transform.position, 10_sch }.draw(Palette::White);
    });
    m_font((m_state == State::Finished) ? U"FINISHED" : U"PLAY  R: Finish").draw(20, 20);
}
