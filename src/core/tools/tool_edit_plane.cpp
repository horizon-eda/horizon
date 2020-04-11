#include "tool_edit_plane.hpp"
#include "board/board_layers.hpp"
#include "document/idocument_board.hpp"
#include "board/board.hpp"
#include "imp/imp_interface.hpp"
#include "pool/part.hpp"
#include <iostream>
#include "core/tool_id.hpp"

namespace horizon {

ToolEditPlane::ToolEditPlane(IDocument *c, ToolID tid) : ToolBase(c, tid)
{
}

Polygon *ToolEditPlane::get_poly()
{
    Polygon *poly = nullptr;
    for (const auto &it : selection) {
        switch (it.type) {
        case ObjectType::POLYGON_ARC_CENTER:
        case ObjectType::POLYGON_EDGE:
        case ObjectType::POLYGON_VERTEX: {
            auto p = doc.b->get_polygon(it.uuid);
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

bool ToolEditPlane::can_begin()
{
    if (!doc.b)
        return false;
    auto poly = get_poly();
    if (!poly)
        return false;
    switch (tool_id) {
    case ToolID::ADD_PLANE:
        return poly->usage == nullptr && BoardLayers::is_copper(poly->layer);

    case ToolID::EDIT_PLANE:
    case ToolID::UPDATE_PLANE:
    case ToolID::CLEAR_PLANE:
        return poly->usage && poly->usage->get_type() == PolygonUsage::Type::PLANE;

    default:
        return false;
    }
}

ToolResponse ToolEditPlane::begin(const ToolArgs &args)
{
    std::cout << "tool edit plane\n";

    auto poly = get_poly();
    Plane *plane = nullptr;
    auto brd = doc.b->get_board();
    if (tool_id == ToolID::EDIT_PLANE || tool_id == ToolID::UPDATE_PLANE || tool_id == ToolID::CLEAR_PLANE) {
        plane = dynamic_cast<Plane *>(poly->usage.ptr);
        if (tool_id == ToolID::UPDATE_PLANE) {
            brd->update_plane(plane);
            return ToolResponse::commit();
        }
        else if (tool_id == ToolID::CLEAR_PLANE) {
            plane->fragments.clear();
            plane->revision++;
            return ToolResponse::commit();
        }
    }
    else {
        auto uu = UUID::random();
        plane = &brd->planes.emplace(uu, uu).first->second;
        plane->polygon = poly;
        poly->usage = plane;
    }
    UUID plane_uuid = plane->uuid;
    bool r = imp->dialogs.edit_plane(plane, brd, doc.b->get_block());
    if (r) {
        if (brd->planes.count(plane_uuid)) // may have been deleted
            brd->update_plane(plane);
        else {
            poly->usage = nullptr;
            brd->update_planes();
        }
        return ToolResponse::commit();
    }
    else {
        return ToolResponse::revert();
    }
}
ToolResponse ToolEditPlane::update(const ToolArgs &args)
{
    return ToolResponse();
}
} // namespace horizon
