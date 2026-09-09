#pragma once

#include "../Common.hpp"

inline String CombinePath(String pathA, String pathB) {
    if (pathA.ends_with(U"/")) {
        return pathA + pathB;
    }
    else {
        return pathA + U"/" + pathB;
    }
}