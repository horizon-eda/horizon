#include "tool_update_all_planes.hpp"
#include "document/idocument_board.hpp"
#include "board/board.hpp"
#include "core/tool_id.hpp"
#include "imp/imp_interface.hpp"

namespace horizon {

bool ToolUpdateAllPlanes::can_begin()
{
    return doc.b;
}

ToolResponse ToolUpdateAllPlanes::begin(const ToolArgs &args)
{
    auto &brd = *doc.b->get_board();
    if (tool_id == ToolID::UPDATE_ALL_PLANES) {
        if (imp->dialogs.update_plane(brd, nullptr))
            return ToolResponse::commit();
        else
            return ToolResponse::revert();
    }
    else if (tool_id == ToolID::CLEAR_ALL_PLANES) {
        std::set<UUID> nets;
        for (auto &it : brd.planes) {
            it.second.clear();
            nets.insert(it.second.net->uuid);
        }
        brd.update_airwires(false, nets);
    }
    return ToolResponse::commit();
}
ToolResponse ToolUpdateAllPlanes::update(const ToolArgs &args)
{
    return ToolResponse();
}
} // namespace horizon
