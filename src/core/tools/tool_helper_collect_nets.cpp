#include "tool_helper_collect_nets.hpp"
#include "document/idocument_board.hpp"
#include "board/board.hpp"

namespace horizon {
std::set<UUID> ToolHelperCollectNets::nets_from_selection(const std::set<SelectableRef> &sel)
{
    std::set<UUID> nets;
    if (!doc.b)
        return nets;
    const auto &brd = *doc.b->get_board();
    for (const auto &it : sel) {
        switch (it.type) {
        case ObjectType::BOARD_PACKAGE: {
            const auto n = brd.packages.at(it.uuid).get_nets();
            nets.insert(n.begin(), n.end());
        } break;

        case ObjectType::JUNCTION: {
            const auto &ju = brd.junctions.at(it.uuid);
            if (ju.net)
                nets.insert(ju.net->uuid);
        } break;

        case ObjectType::TRACK: {
            const auto &tr = brd.tracks.at(it.uuid);
            if (tr.net)
                nets.insert(tr.net->uuid);
        } break;

        case ObjectType::VIA: {
            const auto &via = brd.vias.at(it.uuid);
            if (via.junction->net)
                nets.insert(via.junction->net->uuid);
        } break;

        case ObjectType::POLYGON:
        case ObjectType::POLYGON_EDGE:
        case ObjectType::POLYGON_VERTEX:
        case ObjectType::POLYGON_ARC_CENTER: {
            const auto &poly = brd.polygons.at(it.uuid);
            if (const auto plane = dynamic_cast<const Plane *>(poly.usage.ptr)) {
                nets.insert(plane->net->uuid);
            }
        } break;

        default:;
        }
    }
    return nets;
}

} // namespace horizon
