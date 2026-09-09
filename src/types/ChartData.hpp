#pragma once

#include "../ecs/RhythmComponents.hpp"
#include "../Common.hpp"

struct ChartData {
    std::vector<NoteComponent*> laneLeft;
    std::vector<NoteComponent*> laneRight;
    std::vector<NoteComponent*> laneUp;
    std::vector<NoteComponent*> laneDown;
    double chartOffset = 0.0;
};