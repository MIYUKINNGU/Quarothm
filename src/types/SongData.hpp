#pragma once
#include <string>

struct SongData {
    static const SongData nullSongData;
    unsigned long long id;
    String title;
    String artist;
    String songFilePath;
    std::vector<String> additionalLabels;
    String chartCreatorEasy;
    String chartCreatorNormal;
    String chartCreatorExpert;
    double previewFrom = 0.0;
    double previewTo = 0.0;

    SongData(unsigned long long i, const String& t, const String& a, const String& c, const std::vector<String>& labels,
        const String& easyCreator = U"", const String& normalCreator = U"", const String& expertCreator = U"",
        double preview_from = 0.0, double preview_to = 0.0)
        : title(t), artist(a), songFilePath(c), id(i), additionalLabels(labels),
          chartCreatorEasy(easyCreator), chartCreatorNormal(normalCreator), chartCreatorExpert(expertCreator),
          previewFrom(preview_from), previewTo(preview_to) {}
    
    SongData() : id(0), title(U""), artist(U""), songFilePath(U""), additionalLabels(),
        chartCreatorEasy(U""), chartCreatorNormal(U""), chartCreatorExpert(U"") {}
};

inline const SongData SongData::nullSongData{};
