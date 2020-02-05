#include "tool_update_all_planes.hpp"
#include "core_board.hpp"

namespace horizon {

ToolUpdateAllPlanes::ToolUpdateAllPlanes(IDocument *c, ToolID tid) : ToolBase(c, tid)
{
}

bool ToolUpdateAllPlanes::can_begin()
{
    return core.b;
}

ToolResponse ToolUpdateAllPlanes::begin(const ToolArgs &args)
{
    if (tool_id == ToolID::UPDATE_ALL_PLANES) {
        core.b->get_board()->update_planes();
    }
    else if (tool_id == ToolID::CLEAR_ALL_PLANES) {
        for (auto &it : core.b->get_board()->planes) {
            it.second.fragments.clear();
            it.second.revision++;
        }
    }
    return ToolResponse::commit();
}
ToolResponse ToolUpdateAllPlanes::update(const ToolArgs &args)
{
    return ToolResponse();
}
} // namespace horizon
