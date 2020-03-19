#include "tool_smash_panel_outline.hpp"
#include "document/idocument_board.hpp"
#include "board/board.hpp"
#include <iostream>

namespace horizon {

ToolSmashPanelOutline::ToolSmashPanelOutline(IDocument *c, ToolID tid) : ToolBase(c, tid)
{
}

bool ToolSmashPanelOutline::can_begin()
{
    for (const auto &it : selection) {
        if (it.type == ObjectType::BOARD_PANEL) {
            return doc.b->get_board()->board_panels.at(it.uuid).omit_outline == false;
        }
    }
    return false;
}

ToolResponse ToolSmashPanelOutline::begin(const ToolArgs &args)
{
    for (const auto &it : args.selection) {
        if (it.type == ObjectType::BOARD_PANEL) {
            doc.b->get_board()->smash_panel_outline(doc.b->get_board()->board_panels.at(it.uuid));
        }
    }
    return ToolResponse::commit();
}

ToolResponse ToolSmashPanelOutline::update(const ToolArgs &args)
{
    return ToolResponse();
}
} // namespace horizon
