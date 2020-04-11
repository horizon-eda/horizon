#include "tool_set_diffpair.hpp"
#include "document/idocument_schematic.hpp"
#include "schematic/schematic.hpp"
#include "imp/imp_interface.hpp"
#include <iostream>
#include "core/tool_id.hpp"

namespace horizon {

ToolSetDiffpair::ToolSetDiffpair(IDocument *c, ToolID tid) : ToolBase(c, tid)
{
}

std::pair<Net *, Net *> ToolSetDiffpair::get_net()
{
    std::set<Net *> nets;
    for (const auto &it : selection) {
        if (it.type == ObjectType::JUNCTION) {
            nets.insert(doc.c->get_junction(it.uuid)->net);
        }
        else if (it.type == ObjectType::LINE_NET) {
            nets.insert(doc.c->get_sheet()->net_lines.at(it.uuid).net);
        }
        else if (it.type == ObjectType::POWER_SYMBOL) {
            nets.insert(doc.c->get_sheet()->power_symbols.at(it.uuid).junction->net);
        }
        else if (it.type == ObjectType::NET_LABEL) {
            nets.insert(doc.c->get_sheet()->net_labels.at(it.uuid).junction->net);
        }
    }
    if (nets.size() == 1) {
        return {*nets.begin(), nullptr};
    }
    else if (nets.size() == 2) {
        std::vector<Net *> vnets(nets.begin(), nets.end());
        return {vnets.front(), vnets.back()};
    }
    else {
        return {nullptr, nullptr};
    }
}

bool ToolSetDiffpair::can_begin()
{
    if (!doc.c)
        return false;
    auto nets = get_net();
    if (!nets.first)
        return false;
    if (tool_id == ToolID::SET_DIFFPAIR) {
        if (nets.second) {                                          // two nets have been selected
            return !nets.first->diffpair && !nets.second->diffpair; // no net must be diffpair
        }
        else { // one net
            if (nets.first->diffpair)
                return nets.first->diffpair_master;
            else
                return true;
        }
    }
    else {
        if (nets.second) // clear only likes one net
            return false;
        else
            return nets.first->diffpair;
    }
}

ToolResponse ToolSetDiffpair::begin(const ToolArgs &args)
{
    std::cout << "tool set dp\n";
    auto nets = get_net();

    if (tool_id == ToolID::CLEAR_DIFFPAIR) {
        auto net = nets.first;
        if (net->diffpair_master) {
            net->diffpair = nullptr;
            net->diffpair_master = false;
        }
        else {
            net->diffpair->diffpair = nullptr;
            net->diffpair->diffpair_master = false;
        }
        return ToolResponse::end();
    }

    if (!nets.second) { // only one net
        auto net = nets.first;
        if (net->diffpair && !net->diffpair_master) {
            imp->tool_bar_flash("Net is diffpair slave");
            return ToolResponse::end();
        }

        auto r = imp->dialogs.select_net(doc.c->get_block(), false);
        if (r.first) {
            if (r.second != net->uuid) {
                auto other_net = &doc.c->get_block()->nets.at(r.second);
                if (other_net->diffpair) {
                    imp->tool_bar_flash("Selected net is already a diffpair");
                }
                else {
                    net->diffpair = other_net;
                    net->diffpair_master = true;
                    return ToolResponse::commit();
                }
            }
            else {
                imp->tool_bar_flash("Net can't be its own diffpair");
            }
        }
    }
    else { // two nets
        if (nets.first->diffpair || nets.second->diffpair) {
            imp->tool_bar_flash("Net is already diffpair already");
        }
        nets.first->diffpair_master = true;
        nets.first->diffpair = nets.second;
        return ToolResponse::commit();
    }

    return ToolResponse::end();
}
ToolResponse ToolSetDiffpair::update(const ToolArgs &args)
{
    return ToolResponse();
}
} // namespace horizon
