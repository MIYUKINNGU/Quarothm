#pragma once
#include <string>

struct SongData {
    static const SongData nullSongData;
    unsigned long long id;
    String title;
    String artist;
    String songFilePath;

    SongData(unsigned long long i, const String& t, const String& a, const String& c)
        : title(t), artist(a), songFilePath(c), id(i) {}
};