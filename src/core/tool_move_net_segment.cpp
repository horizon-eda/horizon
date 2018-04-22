#include "tool_move_net_segment.hpp"
#include "core_board.hpp"
#include "core_schematic.hpp"
#include "imp/imp_interface.hpp"
#include <iostream>

namespace horizon {

ToolMoveNetSegment::ToolMoveNetSegment(Core *c, ToolID tid) : ToolBase(c, tid)
{
}

bool ToolMoveNetSegment::can_begin()
{
    if (!core.c) {
        return false;
    }

    return get_net_segment();
}

UUID ToolMoveNetSegment::get_net_segment()
{
    for (const auto &it : core.r->selection) {
        UUID this_ns;
        if (it.type == ObjectType::JUNCTION) {
            this_ns = core.r->get_junction(it.uuid)->net_segment;
        }
        else if (it.type == ObjectType::LINE_NET) {
            this_ns = core.c->get_sheet()->net_lines.at(it.uuid).net_segment;
        }
        else if (it.type == ObjectType::POWER_SYMBOL) {
            this_ns = core.c->get_sheet()->power_symbols.at(it.uuid).junction->net_segment;
        }
        else if (it.type == ObjectType::NET_LABEL) {
            this_ns = core.c->get_sheet()->net_labels.at(it.uuid).junction->net_segment;
        }
        if (this_ns && !net_segment) {
            net_segment = this_ns;
        }
        if (this_ns && net_segment) {
            if (this_ns != net_segment) {
                return UUID();
            }
        }
    }
    return net_segment;
}

ToolResponse ToolMoveNetSegment::begin(const ToolArgs &args)
{
    std::cout << "tool select net seg\n";
    net_segment = get_net_segment();
    core.r->selection.clear();
    if (!net_segment) {
        return ToolResponse::end();
    }


    auto nsinfo = core.c->get_sheet()->analyze_net_segments().at(net_segment);
    if (nsinfo.bus)
        return ToolResponse::end();

    for (const auto &it : core.c->get_sheet()->junctions) {
        if (it.second.net_segment == net_segment) {
            core.c->selection.emplace(it.first, ObjectType::JUNCTION);
        }
    }
    for (const auto &it : core.c->get_sheet()->net_lines) {
        if (it.second.net_segment == net_segment) {
            core.c->selection.emplace(it.first, ObjectType::LINE_NET);
        }
    }
    if (tool_id == ToolID::SELECT_NET_SEGMENT)
        return ToolResponse::end();

    if (tool_id == ToolID::MOVE_NET_SEGMENT_NEW) {
        if (nsinfo.has_power_sym || nsinfo.net->is_bussed) {
            return ToolResponse::end();
        }
        Net *net = core.c->get_schematic()->block->insert_net();
        auto pins = core.c->get_sheet()->get_pins_connected_to_net_segment(net_segment);
        core.c->get_schematic()->block->extract_pins(pins, net);
        core.c->commit();

        return ToolResponse::end();
    }
    if (tool_id == ToolID::MOVE_NET_SEGMENT) {
        if (nsinfo.net->is_bussed) {
            return ToolResponse::end();
        }

        bool r;
        UUID net_uuid;
        std::tie(r, net_uuid) = imp->dialogs.select_net(core.c->get_schematic()->block, nsinfo.net->is_power);
        if (!r) {
            return ToolResponse::end();
        }
        Net *net = &core.c->get_schematic()->block->nets.at(net_uuid);
        auto pins = core.c->get_sheet()->get_pins_connected_to_net_segment(net_segment);
        core.c->get_schematic()->block->extract_pins(pins, net);
        if (nsinfo.net->is_power) {
            for (auto &it : core.c->get_sheet()->power_symbols) {
                if (it.second.junction->net_segment == net_segment) {
                    assert(it.second.net.uuid == nsinfo.net->uuid);
                    it.second.net = net;
                }
            }
        }

        core.c->commit();
    }
    return ToolResponse::end();
}
ToolResponse ToolMoveNetSegment::update(const ToolArgs &args)
{
    return ToolResponse();
}
} // namespace horizon
