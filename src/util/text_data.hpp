#pragma once
#include "common/common.hpp"
#include <vector>
#include "common/lut.hpp"

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
    static const LutEnumStr<TextData::Font> font_lut;

    using Buffer = std::vector<std::pair<Coordi, Coordi>>;
    TextData(Buffer &buf, const std::string &s, Font font = Font::SIMPLEX);
    Buffer &lines;
    int ymin = 0;
    int ymax = 0;
    int xmin = 100;
    int xmax = 0;
    int xright = 0;

    static std::string trim(const std::string &s);
};
} // namespace horizon
