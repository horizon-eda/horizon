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
    void draw_line(const std::deque<Coordf> &pts, ColorP color, uint64_t width);
    void draw_line(const Coordf &from, const Coordf &to, ColorP color, uint64_t width, bool highlight = false,
                   uint8_t color2 = 0);
    void draw_polygon(const std::deque<Coordf> &pts, ColorP color, uint64_t width);
    void draw_arc(const Coordf &center, float radius0, float a0, float a1, ColorP color, uint64_t width);
    void draw_circle(const Coordf &center, float radius0, ColorP color, uint64_t width);
    void draw_curve(const Coordf &start, const Coordf &end, float divation, ColorP color, uint64_t width);
    void draw_bezier2(const Coordf &p0, const Coordf &p1, const Coordf &p2,  ColorP color, uint64_t width);

    bool on_top = true;
    bool use_highlight = false;

private:
    class CanvasGL *ca;
    int layer;
};
} // namespace horizon
