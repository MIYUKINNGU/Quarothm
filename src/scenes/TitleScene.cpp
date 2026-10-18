#include "TitleScene.hpp"

// やることメモ
// 4. チュートリアル楽曲(ただのメトロノーム)を作成(ラベルにtutorialって入れとく)
//   - tutorialのときはテキスト表示関係のjsonを追加する
// 5. 自己べ情報を乗せるか考えて乗せるなら追加する+楽曲タイトルの部分だけ背景色白でちょっとデザイン頑張る

void TitleScene::initialize()
{
    m_creditsOpen = false;
    m_creditsProgress = 0.0;
}

void TitleScene::updateImpl()
{
    if (KeyX.down() || KeyZ.down())
    {
        m_creditsOpen = !m_creditsOpen;
    }

    const double target = m_creditsOpen ? 1.0 : 0.0;
    const double progress_step = std::min(1.0, static_cast<double>(ResponsiveValue(0, 0.35)));
    m_creditsProgress += (target - m_creditsProgress) * progress_step;

    if (m_creditsOpen)
    {
        return;
    }

    if (KeyJ.down() | KeyK.down())
    {
        changeScene(SceneState::SongSelect, 0.3s);
    }
    
    if (KeyEscape.down())
    {
        System::Exit();
    }
}

void TitleScene::drawImpl() const
{
    // "Qua"ttro "ro"tation rhy"thm" game -> Quarothm
    m_titleFont(U"Quarothm").drawAt(Scene::Center().movedBy(0, -45), Palette::Aqua);
    m_guideFont(U"Press ①/② Button").drawAt(Scene::Center().movedBy(0, 45), Palette::White);

    // なんかチグハグ感あったので背景なし
    // Rect{
    //     0, Scene::Height() - 25_sch,
    //     Scene::Width(), 25_sch
    // }.draw(Palette::White);
    m_guideFont(U"+/-：クレジット表記").draw(18.0_sch, Arg::bottomLeft(Vec2{ 13_sch, Scene::Height() - 4_sch }), Palette::White);

    if (m_creditsProgress > 0.0)
    {
        static const std::vector<String> credits{
            U"--- Game Director & Programmer & etc ---",
            U"MIYUKINNGU (Twitter: Scr_MIYUKINNGU2)",
            U"",
            U"--- Charter ---",
            U"MIYUKINNGU",
            U"M1LLI0N PP (EXPERT)",
            U"",
            U"たっくん",
            U"Forest of Clock (NORMAL,EXPERT)",
            U"",
            U"MixingJuice (Twitter: MixingJuice_gen)",
            U"GOODTEK (NORMAL)"
            U"",
            U"--- Music ---",
            U"M1LLI0N PP - Camellia",
            U"GOODTEK - EBIMAYO",
            U"Forest of Clock - ARForest",
            U"Annihilation in F# Minor - tn-shi",
            U"Turkish March - OtoLogic",
            U"(https://otologic.jp/free/bgm/　　　", // Completely Link: https://otologic.jp/free/bgm/wood_mallet-classic-march01.html
            U"　　　　wood_mallet-classic-march01.html)",
            U"2:23 AM - OpenTracks",
            U"(https://opentracks.com/bgm/detail/13494)",
            U"ハンガリー舞曲 第5番 - ポケットサウンド",
            U"(https://pocket-se.info/archives/1615/)"
        };

        const size_t credit_row_count = (credits.size() + 1) / 2;
        const double credit_font_size = std::min(static_cast<double>(24_sch), static_cast<double>(160_sch) / static_cast<double>(credit_row_count + 1));
        const double credit_line_height = credit_font_size * 1.8;
        const double panel_width = 720_sch;
        const double panel_height = 500_sch;
        const double scale = 1.0 + (1.35 - 1.0) * (1.0 - m_creditsProgress);
        const double window_width = panel_width * scale;
        const double window_height = panel_height * scale;
        const double alpha = m_creditsProgress;
        const Vec2 center = Scene::CenterF();

        RectF( 0_sch, 0_sch, Scene::Width(), Scene::Height() ).draw(ColorF{ Color{ U"#000000" }, 0.55 * alpha });
        RoundRect{
            center.x - window_width / 2,
            center.y - window_height / 2,
            window_width,
            window_height,
            4_sch * scale
        }.draw(ColorF{ Color{ U"#464366" }, alpha });

        const double title_y = center.y - window_height / 2 + 45_sch * scale;
        m_guideFont(U"クレジット").draw(24_sch * scale, Arg::center(center.x, title_y), ColorF{ 1.0, 1.0, 1.0, alpha });

        const double first_line_y = title_y + 45_sch * scale;
        const double column_offset = 180_sch * scale;
        for (size_t i = 0; i < credits.size(); ++i)
        {
            const size_t column = i / credit_row_count;
            const size_t row = i % credit_row_count;
            const double column_x = center.x + (column == 0 ? -column_offset : column_offset);
            m_guideFont(credits[i]).draw(
                credit_font_size * scale,
                Arg::center(column_x, first_line_y + credit_line_height * static_cast<double>(row) * scale),
                ColorF{ 0.87, 0.87, 0.87, alpha }
            );
        }
    }
}
