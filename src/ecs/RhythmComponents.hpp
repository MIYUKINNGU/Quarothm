#pragma once

#include <Siv3D.hpp>

struct NoteComponent { double hitTimeSec = 0.0; int32 lane_s = 0; int32 lane_t = 0; bool judged = false; };
struct TransformComponent { Vec2 position; };
