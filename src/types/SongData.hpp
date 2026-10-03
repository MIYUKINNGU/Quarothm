#pragma once
#include <string>

struct SongData {
    static const SongData nullSongData;
    unsigned long long id;
    String title;
    String artist;
    String songFilePath;
    std::vector<String> additionalLabels;

    SongData(unsigned long long i, const String& t, const String& a, const String& c, const std::vector<String>& labels)
        : title(t), artist(a), songFilePath(c), id(i), additionalLabels(labels) {}
    
    SongData() : id(0), title(U""), artist(U""), songFilePath(U""), additionalLabels() {}
};

inline const SongData SongData::nullSongData{};