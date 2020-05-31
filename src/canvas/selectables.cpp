#include "selectables.hpp"
#include "canvas.hpp"

namespace horizon {

Selectable::Selectable(const Coordf &center, const Coordf &box_center, const Coordf &box_dim, float a, bool always)
    : x(center.x), y(center.y), c_x(box_center.x), c_y(box_center.y), width(std::abs(box_dim.x)),
      height(std::abs(box_dim.y)), angle(a), flags(always ? 4 : 0)
{
}

bool Selectable::inside(const Coordf &c, float expand) const
{
    Coordf d = c - Coordf(c_x, c_y);
    float a = -angle;
    float dx = d.x * cos(a) - d.y * sin(a);
    float dy = d.x * sin(a) + d.y * cos(a);
    float w = width / 2;
    float h = height / 2;

    return (dx >= -w - expand) && (dx <= w + expand) && (dy >= -h - expand) && (dy <= h + expand);
}

float Selectable::area() const
{
    if (width == 0)
        return height;
    else if (height == 0)
        return width;
    else
        return width * height;
}

static void rotate(float &x, float &y, float a)
{
    float ix = x;
    float iy = y;
    x = ix * cos(a) - iy * sin(a);
    y = ix * sin(a) + iy * cos(a);
}

std::array<Coordf, 4> Selectable::get_corners() const
{
    std::array<Coordf, 4> r;
    auto w = width + 100;
    auto h = height + 100;
    r[0] = Coordf(-w, -h) / 2;
    r[1] = Coordf(-w, h) / 2;
    r[2] = Coordf(w, h) / 2;
    r[3] = Coordf(w, -h) / 2;
    for (auto &it : r) {
        rotate(it.x, it.y, angle);
        it += Coordf(c_x, c_y);
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

Selectables::Selectables(class Canvas *c) : ca(c)
{
}

void Selectables::append(const UUID &uu, ObjectType ot, const Coordf &center, const Coordf &a, const Coordf &b,
                         unsigned int vertex, int layer, bool always)
{
    Placement tr = ca->transform;
    if (tr.mirror)
        tr.invert_angle();
    tr.mirror = false;
    auto box_center = ca->transform.transform((b + a) / 2);
    auto box_dim = b - a;
    append_angled(uu, ot, center, box_center, box_dim, tr.get_angle_rad(), vertex, layer, always);
}

void Selectables::append(const UUID &uu, ObjectType ot, const Coordf &center, unsigned int vertex, int layer,
                         bool always)
{
    append(uu, ot, center, center, center, vertex, layer, always);
}

void Selectables::append_angled(const UUID &uu, ObjectType ot, const Coordf &center, const Coordf &box_center,
                                const Coordf &box_dim, float angle, unsigned int vertex, int layer, bool always)
{
    items_map.emplace(std::piecewise_construct, std::forward_as_tuple(uu, ot, vertex, layer),
                      std::forward_as_tuple(items.size()));
    items.emplace_back(ca->transform.transform(center), box_center, box_dim, angle, always);
    items_ref.emplace_back(uu, ot, vertex, layer);
}

void Selectables::append_line(const UUID &uu, ObjectType ot, const Coordf &p0, const Coordf &p1, float width,
                              unsigned int vertex, int layer, bool always)
{
    float box_height = width;
    Coordf delta = p1 - p0;
    float box_width = width + sqrt(delta.mag_sq());
    float angle = atan2(delta.y, delta.x);
    auto center = (p0 + p1) / 2;
    append_angled(uu, ot, center, center, Coordf(box_width, box_height), angle, vertex, layer, always);
}


void Selectables::clear()
{
    items.clear();
    items_ref.clear();
    items_map.clear();
}
} // namespace horizon
