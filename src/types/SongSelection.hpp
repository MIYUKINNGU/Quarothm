#pragma once
#include "SongData.hpp"
#include "Difficulty.hpp"
#include "SongsFolderSpecies.hpp"

struct SongSelection {
    unsigned long long selectedSongId;
    SongData selectedSongData;
    SongsFolderSpecies selectedFolderSpecies;
    Difficulty selectedDifficulty;

    SongSelection(const SongData& s, SongsFolderSpecies fs, Difficulty d)
        : selectedSongData(s), selectedFolderSpecies(fs), selectedDifficulty(d), selectedSongId(s.id) {}
};