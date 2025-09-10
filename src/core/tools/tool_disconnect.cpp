#include "tool_disconnect.hpp"
#include "document/idocument_schematic.hpp"
#include "schematic/schematic.hpp"
#include "document/idocument_board.hpp"
#include "board/board.hpp"

namespace horizon {

bool ToolDisconnect::can_begin()
{
    for (const auto &it : selection) {
        if (it.type == ObjectType::SCHEMATIC_SYMBOL) {
            return true;
        }
        else if (it.type == ObjectType::BOARD_PACKAGE) {
            return true;
        }
        else if (it.type == ObjectType::LINE_NET) {
            auto &line = doc.c->get_sheet()->net_lines.at(it.uuid);
            for (auto it_ft : {&line.from, &line.to}) {
                if (it_ft->is_pin())
                    return true;
            }
            return false;
        }
    }
    return false;
}

ToolResponse ToolDisconnect::begin(const ToolArgs &args)
{
    for (const auto &it : args.selection) {
        if (it.type == ObjectType::SCHEMATIC_SYMBOL) {
            doc.c->get_current_schematic()->disconnect_symbol(doc.c->get_sheet(),
                                                              &doc.c->get_sheet()->symbols.at(it.uuid));
        }
        if (it.type == ObjectType::LINE_NET) {
            doc.c->get_current_schematic()->disconnect_net_line(*doc.c->get_sheet(),
                                                                doc.c->get_sheet()->net_lines.at(it.uuid));
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
