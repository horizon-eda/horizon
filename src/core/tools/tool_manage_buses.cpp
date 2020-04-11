#include "tool_manage_buses.hpp"
#include "document/idocument_board.hpp"
#include "board/board.hpp"
#include "document/idocument_schematic.hpp"
#include "document/idocument_frame.hpp"
#include "schematic/schematic.hpp"
#include <iostream>
#include "core/tool_id.hpp"

namespace horizon {

ToolManageBuses::ToolManageBuses(IDocument *c, ToolID tid) : ToolBase(c, tid)
{
}

bool ToolManageBuses::can_begin()
{
    switch (tool_id) {
    case ToolID::MANAGE_BUSES:
    case ToolID::ANNOTATE:
    case ToolID::MANAGE_NET_CLASSES:
    case ToolID::EDIT_SCHEMATIC_PROPERTIES:
    case ToolID::MANAGE_POWER_NETS:
    case ToolID::TOGGLE_GROUP_TAG_VISIBLE:
        return doc.c;

    case ToolID::EDIT_STACKUP:
    case ToolID::MANAGE_INCLUDED_BOARDS:
        return doc.b;

    case ToolID::EDIT_FRAME_PROPERTIES:
        return doc.f;

    default:
        return false;
    }
}

ToolResponse ToolManageBuses::begin(const ToolArgs &args)
{
    bool r = false;

    if (tool_id == ToolID::MANAGE_BUSES) {
        auto sch = doc.c->get_schematic();
        r = imp->dialogs.manage_buses(sch->block);
    }
    else if (tool_id == ToolID::ANNOTATE) {
        auto sch = doc.c->get_schematic();
        r = imp->dialogs.annotate(sch);
    }
    else if (tool_id == ToolID::MANAGE_NET_CLASSES) {
        auto sch = doc.c->get_schematic();
        r = imp->dialogs.manage_net_classes(sch->block);
    }
    else if (tool_id == ToolID::EDIT_SCHEMATIC_PROPERTIES) {
        r = imp->dialogs.edit_schematic_properties(doc.c->get_schematic(), doc.c->get_pool());
    }
    else if (tool_id == ToolID::EDIT_STACKUP) {
        r = imp->dialogs.edit_stackup(doc.b);
    }
    else if (tool_id == ToolID::MANAGE_POWER_NETS) {
        r = imp->dialogs.manage_power_nets(doc.c->get_block());
    }
    else if (tool_id == ToolID::EDIT_FRAME_PROPERTIES) {
        r = imp->dialogs.edit_frame_properties(doc.f->get_frame());
    }
    else if (tool_id == ToolID::TOGGLE_GROUP_TAG_VISIBLE) {
        auto sch = doc.c->get_schematic();
        sch->group_tag_visible = !sch->group_tag_visible;
        r = true;
    }
    else if (tool_id == ToolID::MANAGE_INCLUDED_BOARDS) {
        r = imp->dialogs.manage_included_boards(*doc.b->get_board());
    }
    if (r) {
        return ToolResponse::commit();
    }
    else {
        return ToolResponse::revert();
    }
}

ToolResponse ToolManageBuses::update(const ToolArgs &args)
{
    return ToolResponse();
}
} // namespace horizon
