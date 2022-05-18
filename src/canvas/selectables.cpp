#include "selectables.hpp"
#include "canvas.hpp"
#include <assert.h>
#include "util/geom_util.hpp"

namespace horizon {

Selectable::Selectable(const Coordf &center, const Coordf &box_center, const Coordf &box_dim, float a, bool always)
    : x(center.x), y(center.y), c_x(box_center.x), c_y(box_center.y), width(std::abs(box_dim.x)),
      height(std::abs(box_dim.y)), angle(a), flags(always ? static_cast<int>(Flag::ALWAYS) : 0)
{
}

bool Selectable::inside(const Coordf &c, float expand) const
{
    if (!is_arc()) {
        Coordf d = c - Coordf(c_x, c_y);
        float a = -angle;
        float dx = d.x * cos(a) - d.y * sin(a);
        float dy = d.x * sin(a) + d.y * cos(a);
        float w = std::max(width, expand) / 2;
        float h = std::max(height, expand) / 2;

        return (dx >= -w) && (dx <= w) && (dy >= -h) && (dy <= h);
    }
    else {
        const float r0 = c_x;
        const float r1 = c_y;
        const float rm = (r0 + r1) / 2;
        const float dr = std::max(r1 - r0, expand) / 2;
        const float r0_ex = rm - dr;
        const float r1_ex = rm + dr;

        const float a0 = width;
        const float dphi = height;


        Coordf d = c - get_arc_center();
        auto phi = c2pi(d.angle());
        const float phi0 = c2pi(phi - a0);
        const auto l = d.mag();

        return l > r0_ex && l < r1_ex && phi0 < dphi;
    }
}

float Selectable::area() const
{
    if (is_arc()) {
        const float r0 = c_x;
        const float r1 = c_y;
        const float dphi = height;
        if (r1 != r0) {
            const float a_total = (r1 * r1 - r0 * r0);
            return a_total * (dphi / 2);
        }
        else {
            return r0 * dphi;
        }
    }
    if (width == 0)
        return height;
    else if (height == 0)
        return width;
    else
        return width * height;
}

bool Selectable::is_line() const
{
    return !is_arc() && ((width == 0) != (height == 0));
}

bool Selectable::is_point() const
{
    return !is_arc() && (width == 0 && height == 0);
}

bool Selectable::is_box() const
{
    return !is_arc() && (width > 0 && height > 0);
}

bool Selectable::is_arc() const
{
    return isnan(angle);
}

Coordf Selectable::get_arc_center() const
{
    assert(is_arc());
    if (flags & static_cast<int>(Flag::ARC_CENTER_IS_MIDPOINT)) {
        auto center = Coordf(x, y);
        const float r0 = c_x;
        const float r1 = c_y;
        const float rm = (r0 + r1) / 2;
        const float a0 = width;
        const float dphi = height;
        return center - Coordf::euler(rm, a0 + dphi / 2);
    }
    else {
        return Coordf(x, y);
    }
}


std::array<Coordf, 4> Selectable::get_corners() const
{
    assert(!is_arc());
    std::array<Coordf, 4> r;
    auto w = width + 100;
    auto h = height + 100;
    r[0] = Coordf(-w, -h) / 2;
    r[1] = Coordf(-w, h) / 2;
    r[2] = Coordf(w, h) / 2;
    r[3] = Coordf(w, -h) / 2;
    for (auto &it : r) {
        it = it.rotate(angle) + Coordf(c_x, c_y);
    }
    return r;
}

void Selectable::set_flag(Selectable::Flag f, bool v)
{
    auto mask = static_cast<int>(f);
    if (v) {
        flags |= mask;
    }
    else {
        flags &= ~mask;
    }
}

bool Selectable::get_flag(Selectable::Flag f) const
{
    auto mask = static_cast<int>(f);
    return flags & mask;
}

Selectables::Selectables(const Canvas &c) : ca(c)
{
}

void Selectables::append(const UUID &uu, ObjectType ot, const Coordf &center, const Coordf &a, const Coordf &b,
                         unsigned int vertex, LayerRange layer, bool always)
{
    Placement tr = ca.transform;
    if (tr.mirror)
        tr.invert_angle();
    tr.mirror = false;
    auto box_center = ca.transform.transform((b + a) / 2);
    auto box_dim = b - a;
    append_angled(uu, ot, center, box_center, box_dim, tr.get_angle_rad(), vertex, layer, always);
}

void Selectables::append(const UUID &uu, ObjectType ot, const Coordf &center, unsigned int vertex, LayerRange layer,
                         bool always)
{
    append(uu, ot, center, center, center, vertex, layer, always);
}

void Selectables::append_angled(const UUID &uu, ObjectType ot, const Coordf &center, const Coordf &box_center,
                                const Coordf &box_dim, float angle, unsigned int vertex, LayerRange layer, bool always)
{
    items_map.emplace(std::piecewise_construct, std::forward_as_tuple(uu, ot, vertex, layer),
                      std::forward_as_tuple(items.size()));
    items.emplace_back(ca.transform.transform(center), box_center, box_dim, angle, always);
    items_ref.emplace_back(uu, ot, vertex, layer);
    items_group.push_back(group_current);
}

void Selectables::append_line(const UUID &uu, ObjectType ot, const Coordf &p0, const Coordf &p1, float width,
                              unsigned int vertex, LayerRange layer, bool always)
{
    float box_height = width;
    Coordf delta = p1 - p0;
    float box_width = width + delta.mag();
    float angle = delta.angle();
    auto center = (p0 + p1) / 2;
    append_angled(uu, ot, center, center, Coordf(box_width, box_height), angle, vertex, layer, always);
}

void Selectables::append_arc(const UUID &uu, ObjectType ot, const Coordf &center, float r0, float r1, float a0,
                             float a1, unsigned int vertex, LayerRange layer, bool always)
{
    a0 = c2pi(a0);
    a1 = c2pi(a1);
    const float dphi = c2pi(a1 - a0);
    items_map.emplace(std::piecewise_construct, std::forward_as_tuple(uu, ot, vertex, layer),
                      std::forward_as_tuple(items.size()));
    items.emplace_back(center, Coordf(r0, r1), Coordf(a0, dphi), NAN, always);
    items_ref.emplace_back(uu, ot, vertex, layer);
    items_group.push_back(group_current);
}
void Selectables::append_arc_midpoint(const UUID &uu, ObjectType ot, const Coordf &mid, float r0, float r1, float a0,
                                      float a1, unsigned int vertex, LayerRange layer, bool always)
{
    a0 = c2pi(a0);
    a1 = c2pi(a1);
    const float dphi = c2pi(a1 - a0);
    items_map.emplace(std::piecewise_construct, std::forward_as_tuple(uu, ot, vertex, layer),
                      std::forward_as_tuple(items.size()));
    items.emplace_back(mid, Coordf(r0, r1), Coordf(a0, dphi), NAN, always);
    items.back().set_flag(Selectable::Flag::ARC_CENTER_IS_MIDPOINT, true);
    items_ref.emplace_back(uu, ot, vertex, layer);
    items_group.push_back(group_current);
}

void Selectables::update_preview(const std::set<SelectableRef> &sel)
{
    std::set<int> groups;
    for (const auto &it : sel) {
        if (items_map.count(it)) {
            auto idx = items_map.at(it);
            auto group = items_group.at(idx);
            if (group != -1)
                groups.insert(group);
        }
    }
    for (size_t i = 0; i < items.size(); i++) {
        auto group = items_group.at(i);
        items.at(i).set_flag(Selectable::Flag::PREVIEW, groups.count(group));
    }
}

void Selectables::group_begin()
{
    assert(group_current == -1);
    group_current = group_max;
}

void Selectables::group_end()
{
    assert(group_current != -1);
    group_current = -1;
    group_max++;
}

void Selectables::clear()
{
    items.clear();
    items_ref.clear();
    items_group.clear();
    items_map.clear();
    group_max = 0;
}
} // namespace horizon
