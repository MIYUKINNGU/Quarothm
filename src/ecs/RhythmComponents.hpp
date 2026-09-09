#pragma once

#include <Siv3D.hpp>
#include "../types/NoteType.hpp"

struct NoteComponent { double hitTimeMilliSec = 0.0; int32 lane = 0; NoteType noteType = NoteType::NormalTap; bool judged = false; double holdLength = 0.0; };
struct TransformComponent { Vec2 position; };
