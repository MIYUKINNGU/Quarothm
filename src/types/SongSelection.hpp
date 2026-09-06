#pragma once
#include "SongData.hpp"
#include "Difficulty.hpp"
#include "SongsFolderSpecies.hpp"

struct SongSelection {
    unsigned long long selectedSongId;
    size_t selectedSongIndex;
    SongData selectedSongData;
    SongsFolderSpecies selectedFolderSpecies;
    Difficulty selectedDifficulty;

    SongSelection(const SongData& s, SongsFolderSpecies fs, size_t index, Difficulty d)
        : selectedSongData(s), selectedFolderSpecies(fs), selectedDifficulty(d), selectedSongId(s.id), selectedSongIndex(index) {}
};