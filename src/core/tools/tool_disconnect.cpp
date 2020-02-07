#include "tool_disconnect.hpp"
#include "document/idocument_schematic.hpp"
#include "schematic/schematic.hpp"
#include "document/idocument_board.hpp"
#include "board/board.hpp"
#include <iostream>

namespace horizon {

ToolDisconnect::ToolDisconnect(IDocument *c, ToolID tid) : ToolBase(c, tid)
{
}

bool ToolDisconnect::can_begin()
{
    for (const auto &it : selection) {
        if (it.type == ObjectType::SCHEMATIC_SYMBOL) {
            return true;
        }
        else if (it.type == ObjectType::BOARD_PACKAGE) {
            return true;
        }
    }
    return false;
}

ToolResponse ToolDisconnect::begin(const ToolArgs &args)
{
    std::cout << "tool disconnect\n";
    for (const auto &it : args.selection) {
        if (it.type == ObjectType::SCHEMATIC_SYMBOL) {
            doc.c->get_schematic()->disconnect_symbol(doc.c->get_sheet(), doc.c->get_schematic_symbol(it.uuid));
        }
        else if (it.type == ObjectType::BOARD_PACKAGE) {
            doc.b->get_board()->disconnect_package(&doc.b->get_board()->packages.at(it.uuid));
        }
    }
    return ToolResponse::commit();
}
ToolResponse ToolDisconnect::update(const ToolArgs &args)
{
    return ToolResponse();
}
} // namespace horizon
