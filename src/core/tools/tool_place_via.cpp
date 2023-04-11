#include "tool_place_via.hpp"
#include "document/idocument_board.hpp"
#include "board/board.hpp"
#include "imp/imp_interface.hpp"
#include <iostream>
#include "logger/logger.hpp"
#include "pool/ipool.hpp"
#include "board/board_layers.hpp"

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
    via->net_set = net;
    via->junction->net = net;
    update_via();
}

void ToolPlaceVia::update_via()
{
    if (via_definition) {
        via->source = Via::Source::DEFINITION;
        via->definition = via_definition->uuid;
        via->span = via_definition->span;
        via->pool_padstack = doc.b->get_pool_caching().get_padstack(via_definition->padstack);
        via->parameter_set = via_definition->parameters;
    }
    else {
        via->source = Via::Source::RULES;
        via->parameter_set = rules->get_via_parameter_set(net);
        via->span = BoardLayers::layer_range_through;
        auto ps = doc.b->get_pool_caching().get_padstack(rules->get_via_padstack_uuid(net));
        if (!ps) {
            throw std::runtime_error("padstack not found");
        }
        via->pool_padstack = ps;
    }
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
    std::string tip = net->name;
    if (via_definition)
        tip += " (from definition " + via_definition->name + ")";

    else
        tip += " (from rules)";

    imp->tool_bar_set_tip(tip);
    std::vector<ActionLabelInfo> actions = {
            {InToolActionID::LMB},
            {InToolActionID::RMB},
            {InToolActionID::EDIT, "change net"},
    };
    if (rules->get_rule_t<RuleViaDefinitions>().via_definitions.size()) {
        if (!via_definition) {
            actions.push_back({InToolActionID::SELECT_VIA_DEFINITION, "select via definition"});
        }
        else {
            actions.push_back({InToolActionID::SELECT_VIA_DEFINITION, "use via from rules"});
        }
    }
    imp->tool_bar_set_actions(actions);
}

void ToolPlaceVia::finish()
{
    doc.b->get_board()->airwires_expand = nets;
    doc.b->get_board()->expand_flags = Board::EXPAND_PROPAGATE_NETS | Board::EXPAND_AIRWIRES;
}

bool ToolPlaceVia::update_attached(const ToolArgs &args)
{
    auto &brd = *doc.b->get_board();
    if (args.type == ToolEventType::ACTION) {
        switch (args.action) {
        case InToolActionID::LMB:
            if (args.target.type == ObjectType::JUNCTION) {
                auto j = &brd.junctions.at(args.target.path.at(0));
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
                    via->expand(brd);
                    update_tip();
                }
                return true;
            }
            return false;

        case InToolActionID::SELECT_VIA_DEFINITION:
            if (!rules->get_rule_t<RuleViaDefinitions>().via_definitions.size())
                return false;

            if (via) {
                if (!via_definition) {
                    const auto &rule_via_defs = rules->get_rule_t<RuleViaDefinitions>();
                    if (auto r = imp->dialogs.select_via_definition(rule_via_defs, brd, doc.b->get_pool())) {
                        via_definition = &rule_via_defs.via_definitions.at(*r);
                    }
                }
                else {
                    via_definition = nullptr;
                }
                update_via();
                update_tip();
                return true;
            }
            return false;

        default:;
        }
    }
    return false;
}
} // namespace horizon
