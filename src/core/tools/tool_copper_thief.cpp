#include "tool_copper_thief.hpp"
#include "board/board_layers.hpp"
#include "document/idocument_board.hpp"
#include "board/board.hpp"
#include "imp/imp_interface.hpp"
#include "core/tool_id.hpp"

namespace horizon {

Polygon *ToolCopperThief::get_poly()
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

bool ToolCopperThief::can_begin()
{
    if (!doc.b)
        return false;
    auto poly = get_poly();
    if (!poly)
        return false;
    switch (tool_id) {
    case ToolID::ADD_COPPER_THIEF:
        return poly->usage == nullptr && BoardLayers::is_copper(poly->layer);

    case ToolID::EDIT_COPPER_THIEF:
    case ToolID::UPDATE_COPPER_THIEF:
    case ToolID::CLEAR_COPPER_THIEF:
        return poly->usage && poly->usage->is_type<CopperThief>();

    default:
        return false;
    }
}

ToolResponse ToolCopperThief::begin(const ToolArgs &args)
{
    auto poly = get_poly();
    auto &brd = *doc.b->get_board();
    if (tool_id == ToolID::CLEAR_COPPER_THIEF) {
        auto thief = dynamic_cast<CopperThief *>(poly->usage.ptr);
        thief->clear();
        return ToolResponse::commit();
    }
    if (tool_id == ToolID::UPDATE_COPPER_THIEF) {
        auto thief = dynamic_cast<CopperThief *>(poly->usage.ptr);
        return brd.update_copper_thief(thief) ? ToolResponse::commit() : ToolResponse::revert();
    }
    CopperThief *thief = nullptr;
    const bool add = tool_id == ToolID::ADD_COPPER_THIEF;

    if (add) {
        const auto uu = UUID::random();
        thief = &brd.copper_thieves.emplace(uu, uu).first->second;
        thief->polygon = poly;
        poly->usage = thief;
    }
    else {
        thief = dynamic_cast<CopperThief *>(poly->usage.ptr);
    }

    bool delete_requested = false;
    if (!imp->dialogs.edit_copper_thief(*thief, add, delete_requested)) {
        return ToolResponse::revert();
    }

    if (delete_requested) {
        const auto uu = thief->uuid;
        poly->usage = nullptr;
        brd.copper_thieves.erase(uu);
        return ToolResponse::commit();
    }

    if (!brd.update_copper_thief(thief)) {
        return ToolResponse::revert();
    }
    return ToolResponse::commit();
}

ToolResponse ToolCopperThief::update(const ToolArgs &args)
{
    return ToolResponse();
}

} // namespace horizon
