#pragma once
#include "common/common.hpp"
#include <vector>


namespace horizon {
class TextData {
public:
    enum class Font {
        SMALL,
        SMALL_ITALIC,
        SIMPLEX,
        COMPLEX_SMALL,
        COMPLEX_SMALL_ITALIC,
        DUPLEX,
        COMPLEX,
        COMPLEX_ITALIC,
        TRIPLEX,
        TRIPLEX_ITALIC,
        SCRIPT_SIMPLEX,
        SCRIPT_COMPLEX
    };

    enum class Decoration { NONE = 0, OVERLINE = (1 << 1) };

    TextData(const std::string &s, Font font = Font::SIMPLEX, Decoration decoration = Decoration::NONE);
    std::vector<std::pair<Coordi, Coordi>> lines;
    int ymin = 0;
    int ymax = 0;
    int xmin = 100;
    int xmax = 0;
    int xright = 0;
};
} // namespace horizon
