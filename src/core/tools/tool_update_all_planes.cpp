#include "tool_update_all_planes.hpp"
#include "document/idocument_board.hpp"
#include "board/board.hpp"
#include "core/tool_id.hpp"

namespace horizon {

ToolUpdateAllPlanes::ToolUpdateAllPlanes(IDocument *c, ToolID tid) : ToolBase(c, tid)
{
}

bool ToolUpdateAllPlanes::can_begin()
{
    return doc.b;
}

ToolResponse ToolUpdateAllPlanes::begin(const ToolArgs &args)
{
    if (tool_id == ToolID::UPDATE_ALL_PLANES) {
        doc.b->get_board()->update_planes();
    }
    else if (tool_id == ToolID::CLEAR_ALL_PLANES) {
        for (auto &it : doc.b->get_board()->planes) {
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
