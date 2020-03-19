#include "tool_smash_silkscreen_graphics.hpp"
#include "document/idocument_board.hpp"
#include "board/board.hpp"
#include <iostream>

namespace horizon {

ToolSmashSilkscreenGraphics::ToolSmashSilkscreenGraphics(IDocument *c, ToolID tid) : ToolBase(c, tid)
{
}

bool ToolSmashSilkscreenGraphics::can_begin()
{
    for (const auto &it : selection) {
        if (it.type == ObjectType::BOARD_PACKAGE) {
            return doc.b->get_board()->packages.at(it.uuid).omit_silkscreen == false;
        }
    }
    return false;
}

ToolResponse ToolSmashSilkscreenGraphics::begin(const ToolArgs &args)
{
    for (const auto &it : args.selection) {
        if (it.type == ObjectType::BOARD_PACKAGE) {
            doc.b->get_board()->smash_package_silkscreen_graphics(&doc.b->get_board()->packages.at(it.uuid));
        }
    }
    return ToolResponse::commit();
}
ToolResponse ToolSmashSilkscreenGraphics::update(const ToolArgs &args)
{
    return ToolResponse();
}
} // namespace horizon
