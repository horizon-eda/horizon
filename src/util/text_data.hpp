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
    TextData(const std::string &s, Font font = Font::SIMPLEX);
    std::vector<std::pair<Coordi, Coordi>> lines;
    int ymin = 0;
    int ymax = 0;
    int xmin = 100;
    int xmax = 0;
    int xright = 0;

    static std::string trim(const std::string &s);
};
} // namespace horizon
