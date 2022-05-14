#include "canvas.hpp"
#include "common/common.hpp"
#include "common/text.hpp"
#include "util/placement.hpp"
#include "util/geom_util.hpp"
#include <cstdint>
#include <glib.h>
#include <math.h>
#include <string>
#include <sys/types.h>
#include <tuple>
#include <utility>
#include <vector>

namespace horizon {

void Canvas::set_lod_size(float size)
{
    if (size < 0) {
        lod_current = 0;
    }
    else {
        lod_current = CLAMP(size / 0.02_mm, 1, 255);
        if (lod_current == 255)
            lod_current = 0;
    }
}

void Canvas::draw_line(const Coord<float> &a, const Coord<float> &b, ColorP color, int layer, bool tr, uint64_t width)
{
    if (img_auto_line) {
        Coordi ai(a.x, a.y);
        Coordi bi(b.x, b.y);
        img_line(ai, bi, width, layer, tr);
        return;
    }
    auto pa = a;
    auto pb = b;
    if (tr) {
        pa = transform.transform(a);
        pb = transform.transform(b);
    }
    // ColorP co, uint8_t la, uint32_t oi, uint8_t ty
    add_triangle(layer, pa, pb, Coordf(width, NAN), color, 0);
}

void Canvas::draw_cross(const Coordf &p, float size, ColorP color, int layer, bool tr, uint64_t width)
{
    draw_line(p + Coordf(-size, size), p + Coordf(size, -size), color, layer, tr, width);
    draw_line(p + Coordf(-size, -size), p + Coordf(size, size), color, layer, tr, width);
}
void Canvas::draw_plus(const Coordf &p, float size, ColorP color, int layer, bool tr, uint64_t width)
{
    draw_line(p + Coordf(0, size), p + Coordf(0, -size), color, layer, tr, width);
    draw_line(p + Coordf(-size, 0), p + Coordf(size, 0), color, layer, tr, width);
}
void Canvas::draw_box(const Coordf &p, float size, ColorP color, int layer, bool tr, uint64_t width)
{
    draw_line(p + Coordf(-size, size), p + Coordf(size, size), color, layer, tr, width);
    draw_line(p + Coordf(size, size), p + Coordf(size, -size), color, layer, tr, width);
    draw_line(p + Coordf(size, -size), p + Coordf(-size, -size), color, layer, tr, width);
    draw_line(p + Coordf(-size, -size), p + Coordf(-size, size), color, layer, tr, width);
}

void Canvas::draw_circle(const Coord<float> &center, float radius, ColorP color, int layer)
{
    const auto p0 = center - Coordf(radius, 0);
    const auto p1 = center + Coordf(radius, 0);
    draw_arc(p0, p1, center, color, layer, 0);
    draw_arc(p1, p0, center, color, layer, 0);
}

void Canvas::draw_arc(const Coordf &from, const Coordf &to, const Coordf &center, ColorP color, int layer,
                      uint64_t width)
{
    if (img_auto_line) {
        img_arc(from.to_coordi(), to.to_coordi(), center.to_coordi(), width, layer);
        return;
    }
    Coordf c = project_onto_perp_bisector(from, to, center);
    const float radius0 = (c - from).mag();
    const float a0 = c2pi((from - c).angle());
    const float a1 = c2pi((to - c).angle());
    draw_arc0(c, radius0, a0, a1, color, layer, width);
}

void Canvas::draw_arc0(const Coordf &center, float radius0, float a0, float a1, ColorP color, int layer, uint64_t width)
{

    a1 = c2pi(a1);
    float dphi = c2pi(a1 - a0);

    Coordf p0 = transform.transform(center);
    if (transform.mirror) {
        a0 = -(a0 - M_PI / 2) + M_PI / 2 - dphi;
        a0 -= transform.get_angle_rad();
    }
    else {
        a0 += transform.get_angle_rad();
    }

    a0 = c2pi(a0);
    add_triangle(layer, p0, Coordf(a0, dphi), Coordf(radius0, width), color, TriangleInfo::FLAG_ARC);
}

void Canvas::draw_error(const Coordf &center, float sc, const std::string &text, bool tr)
{
    float x = center.x;
    float y = center.y;
    y -= 3 * sc;
    auto c = ColorP::ERROR;
    const int layer = 10000;
    draw_line({x - 5 * sc, y}, {x + 5 * sc, y}, c, layer, tr);
    draw_line({x - 5 * sc, y}, {x, y + 0.8660f * 10 * sc}, c, layer, tr);
    draw_line({x + 5 * sc, y}, {x, y + 0.8660f * 10 * sc}, c, layer, tr);
    draw_line({x, y + 0.5f * sc}, {x + 1 * sc, y + 1.5f * sc}, c, layer, tr);
    draw_line({x, y + 0.5f * sc}, {x - 1 * sc, y + 1.5f * sc}, c, layer, tr);
    draw_line({x, y + 2.5f * sc}, {x + 1 * sc, y + 1.5f * sc}, c, layer, tr);
    draw_line({x, y + 2.5f * sc}, {x - 1 * sc, y + 1.5f * sc}, c, layer, tr);
    draw_line({x, y + 3 * sc}, {x + 1 * sc, y + 6 * sc}, c, layer, tr);
    draw_line({x, y + 3 * sc}, {x - 1 * sc, y + 6 * sc}, c, layer, tr);
    draw_line({x - 1 * sc, y + 6 * sc}, {x + 1 * sc, y + 6 * sc}, c, layer, tr);
    Coordf text_pos{x - 5 * sc, y - 1.5f * sc};
    if (tr)
        text_pos = transform.transform(text_pos);
    TextRenderer::Options opts;
    opts.flip = get_flip_view();
    draw_text(text_pos, 0.25_mm, text, get_flip_view() ? 32768 : 0, TextOrigin::BASELINE, c, layer, opts);
}

std::tuple<Coordf, Coordf, Coordi> Canvas::draw_flag(const Coordf &position, const std::string &txt, int64_t size,
                                                     Orientation orientation, ColorP c)
{
    Coordi shift;
    int64_t distance = size / 1;
    switch (orientation) {
    case Orientation::LEFT:
        shift.x = -distance;
        break;
    case Orientation::RIGHT:
        shift.x = distance;
        break;
    case Orientation::UP:
        shift.y = distance;
        break;
    case Orientation::DOWN:
        shift.y = -distance;
        break;
    }

    double enlarge = size / 4;

    auto extents =
            draw_text(position + shift, size, txt, orientation_to_angle(orientation), TextOrigin::CENTER, c, 0, {});
    extents.first -= Coordf(enlarge, enlarge);
    extents.second += Coordf(enlarge, enlarge);
    img_auto_line = img_mode;
    switch (orientation) {
    case Orientation::LEFT:
        draw_line(extents.first, {extents.first.x, extents.second.y}, c);
        draw_line({extents.first.x, extents.second.y}, extents.second, c);
        draw_line(extents.second, position, c);
        draw_line(position, {extents.second.x, extents.first.y}, c);
        draw_line({extents.second.x, extents.first.y}, extents.first, c);
        break;

    case Orientation::RIGHT:
        draw_line(extents.second, {extents.second.x, extents.first.y}, c);
        draw_line({extents.first.x, extents.second.y}, extents.second, c);
        draw_line(extents.first, position, c);
        draw_line(position, {extents.first.x, extents.second.y}, c);
        draw_line({extents.second.x, extents.first.y}, extents.first, c);
        break;
    case Orientation::UP:
        draw_line(position, extents.first, c);
        draw_line(position, {extents.second.x, extents.first.y}, c);
        draw_line(extents.first, {extents.first.x, extents.second.y}, c);
        draw_line({extents.second.x, extents.first.y}, extents.second, c);
        draw_line(extents.second, {extents.first.x, extents.second.y}, c);
        break;
    case Orientation::DOWN:
        draw_line(position, extents.second, c);
        draw_line(position, {extents.first.x, extents.second.y}, c);
        draw_line(extents.first, {extents.first.x, extents.second.y}, c);
        draw_line({extents.second.x, extents.first.y}, extents.second, c);
        draw_line(extents.first, {extents.second.x, extents.first.y}, c);
        break;
    }
    img_auto_line = false;
    return std::make_tuple(extents.first, extents.second, shift);
}

void Canvas::draw_lock(const Coordf &center, float size, ColorP color, int layer, bool tr)
{
    float scale = size / 14;
    const std::vector<Coordf> pts = {
            {-6 * scale, -6 * scale}, {6 * scale, -6 * scale}, {6 * scale, 0},           {5 * scale, 1 * scale},
            {-5 * scale, 1 * scale},  {-6 * scale, 0},         {-6 * scale, -6 * scale},
    };
    const std::vector<Coordf> pts2 = {
            {4 * scale, 1 * scale},  {4 * scale, 5 * scale},  {2 * scale, 7 * scale},
            {-2 * scale, 7 * scale}, {-4 * scale, 5 * scale}, {-4 * scale, 1 * scale},
    };
    const std::vector<Coordf> pts3 = {
            {2 * scale, 1 * scale},  {2 * scale, 4 * scale},  {1 * scale, 5 * scale},
            {-1 * scale, 5 * scale}, {-2 * scale, 4 * scale}, {-2 * scale, 1 * scale},
    };
    set_lod_size(size);
    auto sz = pts.size();
    for (size_t i = 0; i < sz; i++) {
        draw_line(center + pts[i], center + pts[(i + 1) % sz], color, layer, tr, 0);
    }
    sz = pts2.size();
    for (size_t i = 1; i < sz; i++) {
        draw_line(center + pts2[i], center + pts2[i - 1], color, layer, tr, 0);
    }
    sz = pts3.size();
    for (size_t i = 1; i < sz; i++) {
        draw_line(center + pts3[i], center + pts3[i - 1], color, layer, tr, 0);
    }
    set_lod_size(-1);
}
} // namespace horizon
