#pragma once
#include <deque>
#include "common/common.hpp"
#include "color_palette.hpp"

namespace horizon {
class CanvasAnnotation {
    friend class CanvasGL;

public:
    CanvasAnnotation(class CanvasGL *c, int l);
    void set_display(const class LayerDisplay &ld);
    void set_visible(bool v);
    void clear();
    void draw_line(const std::deque<Coordi> &pts, ColorP color, uint64_t width);
    void draw_line(const Coordi &from, const Coordi &to, ColorP color, uint64_t width);
    void draw_polygon(const std::deque<Coordi> &pts, ColorP color, uint64_t width);

private:
    class CanvasGL *ca;
    int layer;
};
} // namespace horizon
