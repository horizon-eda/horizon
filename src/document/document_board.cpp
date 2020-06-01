#include "document_board.hpp"
#include "board/board.hpp"

namespace horizon {
std::map<UUID, Polygon> *DocumentBoard::get_polygon_map()
{
    return &get_board()->polygons;
}
std::map<UUID, Junction> *DocumentBoard::get_junction_map()
{
    return &get_board()->junctions;
}
std::map<UUID, Text> *DocumentBoard::get_text_map()
{
    return &get_board()->texts;
}
std::map<UUID, Line> *DocumentBoard::get_line_map()
{
    return &get_board()->lines;
}
std::map<UUID, Arc> *DocumentBoard::get_arc_map()
{
    return &get_board()->arcs;
}
std::map<UUID, Dimension> *DocumentBoard::get_dimension_map()
{
    return &get_board()->dimensions;
}
std::map<UUID, Keepout> *DocumentBoard::get_keepout_map()
{
    return &get_board()->keepouts;
}
std::map<UUID, Picture> *DocumentBoard::get_picture_map()
{
    return &get_board()->pictures;
}

bool DocumentBoard::has_object_type(ObjectType ty) const
{
    switch (ty) {
    case ObjectType::JUNCTION:
    case ObjectType::POLYGON:
    case ObjectType::BOARD_HOLE:
    case ObjectType::TRACK:
    case ObjectType::POLYGON_EDGE:
    case ObjectType::POLYGON_VERTEX:
    case ObjectType::POLYGON_ARC_CENTER:
    case ObjectType::TEXT:
    case ObjectType::LINE:
    case ObjectType::ARC:
    case ObjectType::BOARD_PACKAGE:
    case ObjectType::VIA:
    case ObjectType::DIMENSION:
    case ObjectType::KEEPOUT:
    case ObjectType::CONNECTION_LINE:
    case ObjectType::PICTURE:
        return true;
        break;
    default:;
    }

    return false;
}

std::string DocumentBoard::get_display_name(ObjectType type, const UUID &uu)
{
    switch (type) {
    case ObjectType::BOARD_PACKAGE:
        return get_board()->packages.at(uu).component->refdes;

    case ObjectType::TRACK: {
        const auto &tr = get_board()->tracks.at(uu);
        return tr.net ? tr.net->name : "";
    }

    case ObjectType::VIA: {
        const auto ju = get_board()->vias.at(uu).junction;
        return ju->net ? ju->net->name : "";
    }

    default:
        return Document::get_display_name(type, uu);
    }
}

} // namespace horizon
