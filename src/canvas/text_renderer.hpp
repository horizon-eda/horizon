#pragma once
#include "util/text_renderer.hpp"

namespace horizon {
class CanvasTextRenderer : public TextRenderer {
public:
    CanvasTextRenderer(class Canvas &canvas);

    std::pair<Coordf, Coordf> draw(const Coordf &p, float size, const std::string &rtext, int angle, TextOrigin origin,
                                   ColorP color, int layer, const Options &opts) override;

protected:
    void draw_line(const Coordf &a, const Coordf &b, ColorP color, int layer, uint64_t width) override;

private:
    Canvas &ca;
};
} // namespace horizon
