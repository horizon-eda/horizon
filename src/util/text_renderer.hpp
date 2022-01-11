#pragma once
#include "text_data.hpp"
#include "canvas/color_palette.hpp"
#include "common/common.hpp"
#include "common/text.hpp"

namespace horizon {
class TextRenderer {
public:
    struct Options {
        uint64_t width = 0;
        bool flip = false;
        bool mirror = false;
        bool draw = true;
        TextData::Font font = TextData::Font::SIMPLEX;
        bool center = false;
        bool allow_upside_down = false;
    };


    std::pair<Coordf, Coordf> render(const Text &text, ColorP co, Placement transform, bool rev);

    virtual std::pair<Coordf, Coordf> draw(const Coordf &p, float size, const std::string &rtext, int angle,
                                           TextOrigin origin, ColorP color, int layer, const Options &opts);

protected:
    virtual void draw_line(const Coordf &a, const Coordf &b, ColorP color, int layer, uint64_t width)
    {
    }

private:
    TextData::Buffer buffer;
};
} // namespace horizon
