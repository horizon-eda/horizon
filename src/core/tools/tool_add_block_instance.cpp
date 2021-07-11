#include "tool_add_block_instance.hpp"
#include "document/idocument_schematic.hpp"
#include "schematic/schematic.hpp"
#include "blocks/blocks_schematic.hpp"
#include "imp/imp_interface.hpp"

namespace horizon {

bool ToolAddBlockInstance::can_begin()
{
    return doc.c && (doc.c->get_blocks().blocks.size() > 1);
}

ToolResponse ToolAddBlockInstance::begin(const ToolArgs &args)
{
    if (auto r = imp->dialogs.select_block(doc.c->get_blocks())) {
        auto uu_inst = UUID::random();
        auto &inst = doc.c->get_current_block()
                             ->block_instances
                             .emplace(std::piecewise_construct, std::forward_as_tuple(uu_inst),
                                      std::forward_as_tuple(uu_inst, doc.c->get_blocks().get_block(*r)))
                             .first->second;
        inst.refdes = "I?";
        auto uu_sym = UUID::random();
        temp = &doc.c->get_sheet()
                        ->block_symbols
                        .emplace(std::piecewise_construct, std::forward_as_tuple(uu_sym),
                                 std::forward_as_tuple(uu_sym, doc.c->get_blocks().get_block_symbol(*r), inst))
                        .first->second;
        temp->placement.shift = args.coords;
        temp->schematic = &doc.c->get_blocks().get_schematic(*r);
        doc.c->get_sheet()->expand_block_symbol(uu_sym, *doc.c->get_current_schematic());

        imp->tool_bar_set_actions({
                {InToolActionID::LMB},
                {InToolActionID::RMB},
                {InToolActionID::ROTATE},
                {InToolActionID::MIRROR},
        });
        return ToolResponse();
    }
    else {
        return ToolResponse::end();
    }
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
