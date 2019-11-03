#include "tool_smash_silkscreen_graphics.hpp"
#include "core_board.hpp"
#include <iostream>

namespace horizon {

ToolSmashSilkscreenGraphics::ToolSmashSilkscreenGraphics(Core *c, ToolID tid) : ToolBase(c, tid)
{
}

bool ToolSmashSilkscreenGraphics::can_begin()
{
    for (const auto &it : core.r->selection) {
        if (it.type == ObjectType::BOARD_PACKAGE) {
            return true;
        }
    }
    return false;
}

ToolResponse ToolSmashSilkscreenGraphics::begin(const ToolArgs &args)
{
    for (const auto &it : args.selection) {
        if (it.type == ObjectType::BOARD_PACKAGE) {
            core.b->get_board()->smash_package_silkscreen_graphics(&core.b->get_board()->packages.at(it.uuid));
        }
    }
    core.r->commit();
    return ToolResponse::end();
}
ToolResponse ToolSmashSilkscreenGraphics::update(const ToolArgs &args)
{
    return ToolResponse();
}
} // namespace horizon
