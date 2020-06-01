#include "document.hpp"
#include "common/junction.hpp"
#include "common/line.hpp"
#include "common/arc.hpp"
#include "common/text.hpp"
#include "common/hole.hpp"
#include "common/dimension.hpp"
#include "common/keepout.hpp"
#include "common/picture.hpp"
#include "util/util.hpp"

namespace horizon {

Junction *Document::insert_junction(const UUID &uu)
{
    auto map = get_junction_map();
    auto x = map->emplace(std::make_pair(uu, uu));
    return &(x.first->second);
}

Junction *Document::get_junction(const UUID &uu)
{
    auto map = get_junction_map();
    return &map->at(uu);
}

void Document::delete_junction(const UUID &uu)
{
    auto map = get_junction_map();
    map->erase(uu);
}

Line *Document::insert_line(const UUID &uu)
{
    auto map = get_line_map();
    auto x = map->emplace(std::make_pair(uu, uu));
    return &(x.first->second);
}

Line *Document::get_line(const UUID &uu)
{
    auto map = get_line_map();
    return &map->at(uu);
}

void Document::delete_line(const UUID &uu)
{
    auto map = get_line_map();
    map->erase(uu);
}

std::vector<Line *> Document::get_lines()
{
    auto *map = get_line_map();
    std::vector<Line *> r;
    if (!map)
        return r;
    for (auto &it : *map) {
        r.push_back(&it.second);
    }
    return r;
}

Arc *Document::insert_arc(const UUID &uu)
{
    auto map = get_arc_map();
    auto x = map->emplace(std::make_pair(uu, uu));
    return &(x.first->second);
}

Arc *Document::get_arc(const UUID &uu)
{
    auto map = get_arc_map();
    return &map->at(uu);
}

void Document::delete_arc(const UUID &uu)
{
    auto map = get_arc_map();
    map->erase(uu);
}

std::vector<Arc *> Document::get_arcs()
{
    auto *map = get_arc_map();
    std::vector<Arc *> r;
    if (!map)
        return r;
    for (auto &it : *map) {
        r.push_back(&it.second);
    }
    return r;
}

Text *Document::insert_text(const UUID &uu)
{
    auto map = get_text_map();
    auto x = map->emplace(uu, uu);
    return &(x.first->second);
}

Text *Document::get_text(const UUID &uu)
{
    auto map = get_text_map();
    return &map->at(uu);
}

void Document::delete_text(const UUID &uu)
{
    auto map = get_text_map();
    map->erase(uu);
}

Polygon *Document::insert_polygon(const UUID &uu)
{
    auto map = get_polygon_map();
    auto x = map->emplace(std::make_pair(uu, uu));
    return &(x.first->second);
}

Polygon *Document::get_polygon(const UUID &uu)
{
    auto map = get_polygon_map();
    return &map->at(uu);
}

void Document::delete_polygon(const UUID &uu)
{
    auto map = get_polygon_map();
    map->erase(uu);
}

Hole *Document::insert_hole(const UUID &uu)
{
    auto map = get_hole_map();
    auto x = map->emplace(std::make_pair(uu, uu));
    return &(x.first->second);
}

Hole *Document::get_hole(const UUID &uu)
{
    auto map = get_hole_map();
    return &map->at(uu);
}

void Document::delete_hole(const UUID &uu)
{
    auto map = get_hole_map();
    map->erase(uu);
}

Dimension *Document::insert_dimension(const UUID &uu)
{
    auto map = get_dimension_map();
    auto x = map->emplace(std::make_pair(uu, uu));
    return &(x.first->second);
}

Dimension *Document::get_dimension(const UUID &uu)
{
    auto map = get_dimension_map();
    return &map->at(uu);
}

void Document::delete_dimension(const UUID &uu)
{
    auto map = get_dimension_map();
    map->erase(uu);
}

Keepout *Document::insert_keepout(const UUID &uu)
{
    auto map = get_keepout_map();
    auto x = map->emplace(std::make_pair(uu, uu));
    return &(x.first->second);
}

Keepout *Document::get_keepout(const UUID &uu)
{
    auto map = get_keepout_map();
    return &map->at(uu);
}

void Document::delete_keepout(const UUID &uu)
{
    auto map = get_keepout_map();
    map->erase(uu);
}

std::vector<Keepout *> Document::get_keepouts()
{
    auto *map = get_keepout_map();
    std::vector<Keepout *> r;
    if (!map)
        return r;
    for (auto &it : *map) {
        r.push_back(&it.second);
    }
    return r;
}

Picture *Document::insert_picture(const UUID &uu)
{
    auto map = get_picture_map();
    auto x = map->emplace(std::make_pair(uu, uu));
    return &(x.first->second);
}

Picture *Document::get_picture(const UUID &uu)
{
    auto map = get_picture_map();
    return &map->at(uu);
}

void Document::delete_picture(const UUID &uu)
{
    auto map = get_picture_map();
    map->erase(uu);
}

std::string Document::get_display_name(ObjectType type, const UUID &uu, const UUID &sheet)
{
    return get_display_name(type, uu);
}

std::string Document::get_display_name(ObjectType type, const UUID &uu)
{
    switch (type) {
    case ObjectType::HOLE:
        return get_hole(uu)->shape == Hole::Shape::ROUND ? "Round" : "Slot";

    case ObjectType::TEXT:
        return get_text(uu)->text;

    case ObjectType::DIMENSION: {
        auto dim = get_dimension(uu);
        auto s = dim_to_string(dim->get_length(), false);
        switch (dim->mode) {
        case Dimension::Mode::DISTANCE:
            return s + " D";

        case Dimension::Mode::HORIZONTAL:
            return s + " H";

        case Dimension::Mode::VERTICAL:
            return s + " V";
        }
        return "";
    }

    default:
        return "";
    }
}

} // namespace horizon
