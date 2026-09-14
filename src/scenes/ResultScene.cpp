#include "ResultScene.hpp"

void ResultScene::initialize()
{
    // リザルト表示用の集計・演出の初期化をここへ追加します。
    auto& data = getData();
    unsigned long long id = data.selectedSong.selectedSongId;
    if (data.scoreRecords.find(id) != data.scoreRecords.end()) {
        m_record_score = data.scoreRecords.at(id).score;
        if (data.scoreRecords.at(id).score < data.presentPlayData.score) {
            data.scoreRecords.at(id).score = data.presentPlayData.score;
            data.scoreRecords.at(id).perfectplus = data.presentPlayData.perfectplus;
            data.scoreRecords.at(id).perfect = data.presentPlayData.perfect;
            data.scoreRecords.at(id).good = data.presentPlayData.good;
            data.scoreRecords.at(id).miss = data.presentPlayData.miss;
        }
        data.scoreRecords.at(id).accuracy = std::max(data.scoreRecords.at(id).accuracy, data.presentPlayData.accuracy);
        data.scoreRecords.at(id).maxCombo = std::max(data.scoreRecords.at(id).maxCombo, data.presentPlayData.maxCombo);
        data.scoreRecords.at(id).clear = std::max(data.scoreRecords.at(id).clear, data.presentPlayData.clear);
    }
    else {
        data.scoreRecords[id] = ScoreRecorder {
            data.presentPlayData.clear,
            data.presentPlayData.score,
            data.presentPlayData.maxCombo,
            data.presentPlayData.accuracy,
            data.presentPlayData.perfectplus,
            data.presentPlayData.perfect,
            data.presentPlayData.good,
            data.presentPlayData.miss
        };
    }

    JSON scoreRecordsJson;

    for (auto& [id, data] : data.scoreRecords) {
        JSON scoreJson;
        scoreJson[U"id"] = id;
        scoreJson[U"clear"] = static_cast<int32>(data.clear);
        scoreJson[U"score"] = data.score;
        scoreJson[U"maxCombo"] = data.maxCombo;
        scoreJson[U"accuracy"] = data.accuracy;
        scoreJson[U"perfectplus"] = data.perfectplus;
        scoreJson[U"perfect"] = data.perfect;
        scoreJson[U"good"] = data.good;
        scoreJson[U"miss"] = data.miss;
        
        scoreRecordsJson.push_back(scoreJson);
    }
    scoreRecordsJson.save(U"./saves/play_records.json");
}

void ResultScene::updateImpl()
{
    if (KeyEnter.down()) changeScene(SceneState::SongSelect, 0.0s);
    else if (KeyEscape.down()) changeScene(SceneState::Title, 0.0s);
}

void ResultScene::drawImpl() const
{
    auto& data = getData();
    Point center = Scene::Center();
    unsigned long long id = data.selectedSong.selectedSongId;
    unsigned int score = data.presentPlayData.score;
    double accuracy = data.presentPlayData.accuracy;

    long long diff = (long long)score-(long long)m_record_score;
    m_headingFont(U"RESULT").draw(42_sch, Arg::center(center.x, center.y - 200_sch), Palette::Yellow);
    m_bodyFont(U"{:09}"_fmt(score)).draw(30_sch, Arg::center(center.x, center.y - 145_sch));
    m_bodyFont(U"{:.2f}%"_fmt(accuracy)).draw(20_sch, Arg::rightCenter(center.x + 200_sch, center.y - 145_sch));
    m_bodyFont(U"High Score").draw(16_sch, Arg::leftCenter(center.x - 200_sch, center.y - 100_sch));
    m_bodyFont(U"{:09}"_fmt(m_record_score)).draw(23_sch, Arg::center(center.x, center.y - 100_sch));
    m_bodyFont((diff > 0 ? U"+" : U"-")+U"{:09}"_fmt(std::abs(diff))).draw(16_sch, Arg::rightCenter(center.x + 200_sch, center.y - 100_sch));
    m_bodyFont(U"MAX COMBO " + Format(data.presentPlayData.maxCombo)).draw(20_sch, Arg::center(center.x, center.y - 75_sch));

    m_bodyFont(U"Perfect+").draw(TextStyle::Outline(0.1_sch, Palette::White), 16_sch, Arg::leftCenter(center.x - 75_sch, center.y - 34_sch), Palette::Pink);
    m_bodyFont(U"{}"_fmt(data.presentPlayData.perfectplus)).draw(16_sch, Arg::rightCenter(center.x + 75_sch, center.y - 34_sch));
    m_bodyFont(U"Perfect").draw(TextStyle::Outline(0.1_sch, Palette::White), 16_sch, Arg::leftCenter(center.x - 75_sch, center.y), Palette::Purple);
    m_bodyFont(U"{}"_fmt(data.presentPlayData.perfect)).draw(16_sch, Arg::rightCenter(center.x + 75_sch, center.y));
    m_bodyFont(U"Good").draw(TextStyle::Outline(0.1_sch, Palette::White), 16_sch, Arg::leftCenter(center.x - 75_sch, center.y + 34_sch), Palette::Orange);
    m_bodyFont(U"{}"_fmt(data.presentPlayData.good)).draw(16_sch, Arg::rightCenter(center.x + 75_sch, center.y + 34_sch));
    m_bodyFont(U"Miss").draw(TextStyle::Outline(0.1_sch, Palette::White), 16_sch, Arg::leftCenter(center.x - 75_sch, center.y + 68_sch), Palette::Red);
    m_bodyFont(U"{}"_fmt(data.presentPlayData.miss)).draw(16_sch, Arg::rightCenter(center.x + 75_sch, center.y + 68_sch));

    m_bodyFont(U"Enter: Song Select    Esc: Title").draw(26_sch, Arg::center(center.x, center.y + 120_sch));
}
