#include "tool_place_via.hpp"
#include "document/idocument_board.hpp"
#include "board/board.hpp"
#include "imp/imp_interface.hpp"
#include <iostream>
#include "logger/logger.hpp"
#include "pool/ipool.hpp"

namespace horizon {

bool ToolPlaceVia::can_begin()
{
    return doc.b;
}

bool ToolPlaceVia::begin_attached()
{
    rules = dynamic_cast<BoardRules *>(doc.b->get_rules());
    if (auto r = imp->dialogs.select_net(*doc.b->get_top_block(), false)) {
        net = doc.b->get_top_block()->get_net(*r);
        nets.insert(net->uuid);
        auto ps_uu = rules->get_via_padstack_uuid(net);
        if (!ps_uu) {
            Logger::log_warning("Didn't find a via for net " + net->name, Logger::Domain::TOOL,
                                "make sure that there's a default via rule");
            return false;
        }
        update_tip();
        imp->tool_bar_set_actions({
                {InToolActionID::LMB},
                {InToolActionID::RMB},
                {InToolActionID::EDIT, "change net"},
        });
        return true;
    }
    else {
        return false;
    }
}

void ToolPlaceVia::insert_junction()
{
    const auto uu = UUID::random();
    temp = &doc.b->get_board()->junctions.emplace(uu, uu).first->second;
}

void ToolPlaceVia::create_attached()
{
    auto uu = UUID::random();
    auto ps = doc.b->get_pool_caching().get_padstack(rules->get_via_padstack_uuid(net));
    if (!ps) {
        throw std::runtime_error("padstack not found");
    }
    via = &doc.b->get_board()
                   ->vias.emplace(std::piecewise_construct, std::forward_as_tuple(uu), std::forward_as_tuple(uu, ps))
                   .first->second;
    via->junction = temp;
    via->source = Via::Source::RULES;
    via->net_set = net;
    via->junction->net = net;
    via->parameter_set = rules->get_via_parameter_set(net);
    via->expand(*doc.b->get_board());
}

void ToolPlaceVia::delete_attached()
{
    if (via) {
        doc.b->get_board()->vias.erase(via->uuid);
    }
}

void ToolPlaceVia::update_tip()
{
    imp->tool_bar_set_tip(net->name);
}

void ToolPlaceVia::finish()
{
    doc.b->get_board()->airwires_expand = nets;
    doc.b->get_board()->expand_flags = Board::EXPAND_PROPAGATE_NETS | Board::EXPAND_AIRWIRES;
}

bool ToolPlaceVia::update_attached(const ToolArgs &args)
{
    if (args.type == ToolEventType::ACTION) {
        switch (args.action) {
        case InToolActionID::LMB:
            if (args.target.type == ObjectType::JUNCTION) {
                auto j = &doc.b->get_board()->junctions.at(args.target.path.at(0));
                via->junction = j;
                create_attached();
                return true;
            }
            return false;

        case InToolActionID::EDIT:
            if (via) {
                if (auto r = imp->dialogs.select_net(*doc.b->get_top_block(), false)) {
                    net = doc.b->get_top_block()->get_net(*r);
                    nets.insert(net->uuid);
                    via->net_set = net;
                    via->parameter_set = rules->get_via_parameter_set(net);
                    via->expand(*doc.b->get_board());
                    update_tip();
                }
                return true;
            }
            return false;

        default:;
        }
    }
    return false;
}
} // namespace horizon
