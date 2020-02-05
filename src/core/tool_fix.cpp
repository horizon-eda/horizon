#include "tool_fix.hpp"
#include "idocument_board.hpp"
#include "board/board.hpp"
#include <iostream>

namespace horizon {

ToolFix::ToolFix(IDocument *c, ToolID tid) : ToolBase(c, tid)
{
}

bool ToolFix::can_begin()
{
    for (const auto &it : selection) {
        if (it.type == ObjectType::BOARD_PACKAGE) {
            auto pkg = &core.b->get_board()->packages.at(it.uuid);
            if (pkg->fixed == (tool_id == ToolID::UNFIX))
                return true;
        }
    }
    return false;
}

ToolResponse ToolFix::begin(const ToolArgs &args)
{
    for (const auto &it : args.selection) {
        if (it.type == ObjectType::BOARD_PACKAGE) {
            core.b->get_board()->packages.at(it.uuid).fixed = tool_id == ToolID::FIX;
        }
    }
    return ToolResponse::commit();
}
ToolResponse ToolFix::update(const ToolArgs &args)
{
    return ToolResponse();
}
} // namespace horizon
