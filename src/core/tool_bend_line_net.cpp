#include "tool_bend_line_net.hpp"
#include "core_schematic.hpp"
#include "imp/imp_interface.hpp"
#include <iostream>

namespace horizon {

ToolBendLineNet::ToolBendLineNet(Core *c, ToolID tid) : ToolBase(c, tid)
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

    temp = core.c->insert_junction(UUID::random());
    temp->temp = true;
    temp->position = args.coords;

    LineNet *li = &core.c->get_sheet()->net_lines.at(it->uuid);
    core.c->get_sheet()->split_line_net(li, temp);
    imp->tool_bar_set_tip("<b>LMB:</b>place junction <b>RMB:</b>cancel");
    return ToolResponse();
}
ToolResponse ToolBendLineNet::update(const ToolArgs &args)
{
    if (args.type == ToolEventType::MOVE) {
        temp->position = args.coords;
    }
    else if (args.type == ToolEventType::CLICK) {
        temp->temp = false;
        if (args.button == 1) {
            core.c->commit();
            return ToolResponse::end();
        }
        else if (args.button == 3) {
            core.c->revert();
            return ToolResponse::end();
        }
    }
    else if (args.type == ToolEventType::KEY) {
        if (args.key == GDK_KEY_Escape) {
            core.c->revert();
            return ToolResponse::end();
        }
    }
    return ToolResponse();
}
} // namespace horizon
