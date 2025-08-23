#include "tool_add_height_restriction.hpp"
#include "board/board_layers.hpp"
#include "document/idocument_board.hpp"
#include "board/board.hpp"
#include "document/idocument_package.hpp"
#include "pool/package.hpp"
#include "imp/imp_interface.hpp"
#include "pool/part.hpp"
#include <iostream>
#include "core/tool_id.hpp"

namespace horizon {

Polygon *ToolAddHeightRestriction::get_poly()
{
    Polygon *poly = nullptr;
    for (const auto &it : selection) {
        switch (it.type) {
        case ObjectType::POLYGON_ARC_CENTER:
        case ObjectType::POLYGON_EDGE:
        case ObjectType::POLYGON_VERTEX: {
            auto p = doc.r->get_polygon(it.uuid);
            if (poly && poly != p) {
                return nullptr;
            }
            else {
                poly = p;
            }
        } break;
        default:;
        }
    }
    return poly;
}

bool ToolAddHeightRestriction::can_begin()
{
    if (!doc.b)
        return false;
    auto poly = get_poly();
    if (!poly)
        return false;
    switch (tool_id) {
    case ToolID::ADD_HEIGHT_RESTRICTION:
        return poly->usage == nullptr;

    case ToolID::DELETE_HEIGHT_RESTRICTION:
        return poly->usage->is_type<HeightRestriction>();

    default:
        return false;
    }
}

ToolResponse ToolAddHeightRestriction::begin(const ToolArgs &args)
{
    auto poly = get_poly();
    auto &brd = *doc.b->get_board();

    if (tool_id == ToolID::DELETE_KEEPOUT) {
        brd.keepouts.erase(poly->usage->get_uuid());
        poly->usage = nullptr;
    }
    else {
        const auto uu = UUID::random();
        auto &hr = brd.height_restrictions.emplace(uu, uu).first->second;
        hr.polygon = poly;
        poly->usage = &hr;
    }

    return ToolResponse::commit();
}

ToolResponse ToolAddHeightRestriction::update(const ToolArgs &args)
{
    return ToolResponse();
}
} // namespace horizon
