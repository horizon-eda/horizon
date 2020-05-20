#include "tool_bend_line_net.hpp"
#include "document/idocument_schematic.hpp"
#include "schematic/schematic.hpp"
#include "imp/imp_interface.hpp"
#include <iostream>
#include <gdk/gdkkeysyms.h>

namespace horizon {

ToolBendLineNet::ToolBendLineNet(IDocument *c, ToolID tid) : ToolBase(c, tid)
{
}

bool ToolBendLineNet::can_begin()
{
    if (selection.size() != 1)
        return false;

    const auto it = selection.begin();
    if (it->type != ObjectType::LINE_NET)
        return false;
    return true;
}

ToolResponse ToolBendLineNet::begin(const ToolArgs &args)
{
    std::cout << "tool bend net line\n";

    if (args.selection.size() != 1)
        return ToolResponse::end();

    const auto it = args.selection.begin();
    if (it->type != ObjectType::LINE_NET)
        return ToolResponse::end();

    temp = doc.c->insert_junction(UUID::random());
    temp->position = args.coords;

    LineNet *li = &doc.c->get_sheet()->net_lines.at(it->uuid);
    doc.c->get_sheet()->split_line_net(li, temp);
    imp->tool_bar_set_tip("<b>LMB:</b>place junction <b>RMB:</b>cancel");
    return ToolResponse();
}
ToolResponse ToolBendLineNet::update(const ToolArgs &args)
{
    if (args.type == ToolEventType::MOVE) {
        temp->position = args.coords;
    }
    else if (args.type == ToolEventType::CLICK) {
        if (args.button == 1) {
            return ToolResponse::commit();
        }
        else if (args.button == 3) {
            return ToolResponse::revert();
        }
    }
    else if (args.type == ToolEventType::KEY) {
        if (args.key == GDK_KEY_Escape) {
            return ToolResponse::revert();
        }
    }
    return ToolResponse();
}
} // namespace horizon
