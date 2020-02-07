#include "tool_swap_nets.hpp"
#include "document/idocument_schematic.hpp"
#include "schematic/schematic.hpp"
#include "imp/imp_interface.hpp"
#include <iostream>

namespace horizon {

ToolSwapNets::ToolSwapNets(IDocument *c, ToolID tid) : ToolBase(c, tid)
{
}

bool ToolSwapNets::can_begin()
{
    if (!doc.c) {
        return false;
    }

    return get_net_segments().size() == 2;
}

std::set<UUID> ToolSwapNets::get_net_segments()
{
    std::set<UUID> net_segments;
    for (const auto &it : selection) {
        if (it.type == ObjectType::JUNCTION) {
            net_segments.insert(doc.r->get_junction(it.uuid)->net_segment);
        }
        else if (it.type == ObjectType::LINE_NET) {
            net_segments.insert(doc.c->get_sheet()->net_lines.at(it.uuid).net_segment);
        }
        else if (it.type == ObjectType::POWER_SYMBOL) {
            net_segments.insert(doc.c->get_sheet()->power_symbols.at(it.uuid).junction->net_segment);
        }
        else if (it.type == ObjectType::NET_LABEL) {
            net_segments.insert(doc.c->get_sheet()->net_labels.at(it.uuid).junction->net_segment);
        }
    }
    return net_segments;
}

ToolResponse ToolSwapNets::begin(const ToolArgs &args)
{

    UUID ns1;
    UUID ns2;
    {
        auto net_segments = get_net_segments();
        auto it = net_segments.begin();
        ns1 = *it;
        it++;
        ns2 = *it;
    }

    auto nsinfo = doc.c->get_sheet()->analyze_net_segments();
    auto nsi1 = nsinfo.at(ns1);
    auto nsi2 = nsinfo.at(ns2);

    if (nsi1.bus || nsi1.has_power_sym || nsi1.net->is_bussed || nsi2.bus || nsi2.has_power_sym || nsi2.net->is_bussed)
        return ToolResponse::end();

    auto pins1 = doc.c->get_sheet()->get_pins_connected_to_net_segment(ns1);
    auto pins2 = doc.c->get_sheet()->get_pins_connected_to_net_segment(ns2);

    doc.c->get_schematic()->block->extract_pins(pins1, nsi2.net);
    doc.c->get_schematic()->block->extract_pins(pins2, nsi1.net);

    return ToolResponse::commit();
}
ToolResponse ToolSwapNets::update(const ToolArgs &args)
{
    return ToolResponse();
}
} // namespace horizon
