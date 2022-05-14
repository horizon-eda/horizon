#include "features.hpp"
#include "odb_util.hpp"
#include "symbol.hpp"
#include "util/geom_util.hpp"
#include "common/polygon.hpp"
#include "common/shape.hpp"
#include <sstream>

namespace horizon::ODB {

unsigned int Features::get_or_create_symbol_circle(uint64_t diameter)
{
    return get_or_create_symbol(symbols_circle, diameter);
}

unsigned int Features::get_or_create_symbol_pad(const std::string &sym)
{
    return get_or_create_symbol(symbols_pad, sym);
}

unsigned int Features::get_or_create_symbol_rect(uint64_t width, uint64_t height)
{
    return get_or_create_symbol(symbols_rect, std::make_pair(width, height));
}

unsigned int Features::get_or_create_symbol_oval(uint64_t width, uint64_t height)
{
    return get_or_create_symbol(symbols_oval, std::make_pair(width, height));
}

Features::Line &Features::draw_line(const Coordi &from, const Coordi &to, uint64_t width)
{
    const auto sym = get_or_create_symbol_circle(width);
    return add_feature<Line>(from, to, sym);
}

Features::Arc &Features::draw_arc(const Coordi &from, const Coordi &to, const Coordi &center, uint64_t width,
                                  Arc::Direction direction)
{
    const auto sym = get_or_create_symbol_circle(width);
    const auto real_center = project_onto_perp_bisector(from, to, center).to_coordi();
    return add_feature<Arc>(from, to, real_center, sym, direction);
}

std::vector<Features::Feature *> Features::draw_polygon_outline(const Polygon &ipoly, const Placement &transform)
{
    std::vector<Features::Feature *> r;
    r.reserve(ipoly.vertices.size());
    for (size_t i = 0; i < ipoly.vertices.size(); i++) {
        const auto &v_last = ipoly.get_vertex(i);
        const auto &v = ipoly.get_vertex(i + 1);
        if (v_last.type == Polygon::Vertex::Type::LINE) {
            r.push_back(&draw_line(transform.transform(v_last.position), transform.transform(v.position), 0));
        }
        else if (v_last.type == Polygon::Vertex::Type::ARC) {
            r.push_back(&draw_arc(transform.transform(v_last.position), transform.transform(v.position),
                                  transform.transform(v_last.arc_center), 0,
                                  v_last.arc_reverse != transform.mirror ? Arc::Direction::CW : Arc::Direction::CCW));
        }
    }
    return r;
}

Features::Pad &Features::draw_pad(const std::string &sym, const Placement &transform)
{
    return add_feature<Pad>(transform, get_or_create_symbol_pad(sym));
}

Features::Pad &Features::draw_circle(const Coordi &pos, uint64_t diameter)
{
    return add_feature<Pad>(Placement(pos), get_or_create_symbol_circle(diameter));
}

Features::Pad &Features::draw_shape(const Shape &shape)
{
    switch (shape.form) {
    case Shape::Form::CIRCLE:
        return add_feature<Pad>(shape.placement, get_or_create_symbol_circle(shape.params.at(0)));

    case Shape::Form::RECTANGLE:
        return add_feature<Pad>(shape.placement, get_or_create_symbol_rect(shape.params.at(0), shape.params.at(1)));

    case Shape::Form::OBROUND:
        return add_feature<Pad>(shape.placement, get_or_create_symbol_oval(shape.params.at(0), shape.params.at(1)));
    }
    throw std::runtime_error("unsupported shape form");
}

Features::Surface &Features::add_surface()
{
    return add_feature<Surface>();
}

void Features::Feature::write(std::ostream &ost) const
{
    switch (get_type()) {
    case Type::LINE:
        ost << "L";
        break;

    case Type::ARC:
        ost << "A";
        break;

    case Type::PAD:
        ost << "P";
        break;

    case Type::SURFACE:
        ost << "S";
        break;
    }
    ost << " ";
    write_feature(ost);

    write_attributes(ost);
    ost << endl;
}

void Features::Line::write_feature(std::ostream &ost) const
{
    ost << from << " " << to << " " << symbol << " P 0";
}

void Features::Arc::write_feature(std::ostream &ost) const
{
    ost << from << " " << to << " "
        << " " << center << " " << symbol << " P 0 " << (direction == Direction::CW ? "Y" : "N");
}

void Features::Pad::write_feature(std::ostream &ost) const
{
    ost << placement.shift << " " << symbol << " P 0 ";
    if (placement.mirror)
        ost << "9";
    else
        ost << "8";
    ost << " " << Angle{placement};
}

void Features::Surface::write_feature(std::ostream &ost) const
{
    ost << "P 0";
}

void Features::Surface::write(std::ostream &ost) const
{
    Features::Feature::write(ost);
    data.write(ost);
    ost << "SE" << endl;
}

void Features::write(std::ostream &ost) const
{
    if (features.size() == 0)
        return;
    ost << "UNITS=MM" << endl;
    ost << "#Symbols" << endl;
    for (const auto &[diameter, n] : symbols_circle) {
        ost << "$" << n << " " << make_symbol_circle(diameter) << endl;
    }
    for (const auto &[dim, n] : symbols_rect) {
        ost << "$" << n << " " << make_symbol_rect(dim.first, dim.second) << endl;
    }
    for (const auto &[dim, n] : symbols_oval) {
        ost << "$" << n << " " << make_symbol_oval(dim.first, dim.second) << endl;
    }
    for (const auto &[name, n] : symbols_pad) {
        ost << "$" << n << " " << name << endl;
    }
    write_attributes(ost);
    for (const auto &feat : features) {
        feat->write(ost);
    }
}
} // namespace horizon::ODB
