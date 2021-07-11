#include "tool_add_block_instance.hpp"
#include "document/idocument_schematic.hpp"
#include "schematic/schematic.hpp"
#include "blocks/blocks_schematic.hpp"
#include "imp/imp_interface.hpp"
#include "util/selection_util.hpp"

namespace horizon {

bool ToolAddBlockInstance::can_begin()
{
    return doc.c && (doc.c->get_blocks().blocks.size() > 1);
}

ToolResponse ToolAddBlockInstance::begin(const ToolArgs &args)
{
    UUID block;
    if (auto s = sel_find_exactly_one(selection, ObjectType::BLOCK)) {
        block = s->uuid;
    }
    else {
        if (auto r = imp->dialogs.select_block(doc.c->get_blocks())) {
            block = *r;
        }
        else {
            return ToolResponse::end();
        }
    }
    if (!doc.c->get_top_block()->can_add_block_instance(doc.c->get_current_block()->uuid, block)) {
        imp->tool_bar_flash("can't place this block here");
        return ToolResponse::end();
    }
    auto uu_inst = UUID::random();
    auto &inst = doc.c->get_current_block()
                         ->block_instances
                         .emplace(std::piecewise_construct, std::forward_as_tuple(uu_inst),
                                  std::forward_as_tuple(uu_inst, doc.c->get_blocks().get_block(block)))
                         .first->second;
    inst.refdes = "I?";
    auto uu_sym = UUID::random();
    temp = &doc.c->get_sheet()
                    ->block_symbols
                    .emplace(std::piecewise_construct, std::forward_as_tuple(uu_sym),
                             std::forward_as_tuple(uu_sym, doc.c->get_blocks().get_block_symbol(block), inst))
                    .first->second;
    temp->placement.shift = args.coords;
    temp->schematic = &doc.c->get_blocks().get_schematic(block);
    doc.c->get_sheet()->expand_block_symbol(uu_sym, *doc.c->get_current_schematic());
    doc.c->get_top_schematic()->update_sheet_mapping();

    imp->tool_bar_set_actions({
            {InToolActionID::LMB},
            {InToolActionID::RMB},
            {InToolActionID::ROTATE},
            {InToolActionID::MIRROR},
    });
    return ToolResponse();
}


ToolResponse ToolAddBlockInstance::update(const ToolArgs &args)
{

    if (args.type == ToolEventType::MOVE) {
        temp->placement.shift = args.coords;
    }
    else if (args.type == ToolEventType::ACTION) {
        switch (args.action) {
        case InToolActionID::LMB:
            return ToolResponse::commit();
            break;

        case InToolActionID::RMB:
        case InToolActionID::CANCEL:
            return ToolResponse::revert();

        case InToolActionID::ROTATE:
            if (temp->placement.mirror) {
                temp->placement.inc_angle_deg(90);
            }
            else {
                temp->placement.inc_angle_deg(-90);
            }
            break;

        case InToolActionID::MIRROR:
            temp->placement.mirror = !temp->placement.mirror;

        default:;
        }
    }
    return ToolResponse();
}
} // namespace horizon
