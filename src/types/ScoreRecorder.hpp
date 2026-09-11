#pragma once

#include "ClearType.hpp"

struct ScoreRecorder
{
    ClearType clear = ClearType::None;
    unsigned int score = 0;
    unsigned int maxCombo = 0;

    unsigned int perfectplus = 0;
    unsigned int perfect = 0;
    unsigned int good = 0;
    unsigned int miss = 0;
};