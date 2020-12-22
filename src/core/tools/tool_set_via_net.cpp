#include "tool_set_via_net.hpp"
#include "document/idocument_board.hpp"
#include "board/board.hpp"
#include "imp/imp_interface.hpp"
#include <iostream>
#include "core/tool_id.hpp"

namespace horizon {

std::set<Via *> ToolSetViaNet::get_vias()
{
    std::set<Via *> vias;
    for (const auto &it : selection) {
        if (it.type == ObjectType::VIA) {
            auto via = &doc.b->get_board()->vias.at(it.uuid);
            if (tool_id == ToolID::SET_VIA_NET) {
                if (via->net_set == via->junction->net) {
                    vias.insert(via);
                }
            }
            else {
                if (via->net_set) {
                    vias.insert(via);
                }
            }
        }
    }
    return vias;
}

bool ToolSetViaNet::can_begin()
{
    if (!doc.b)
        return false;
    return get_vias().size() > 0;
}

ToolResponse ToolSetViaNet::begin(const ToolArgs &args)
{
    auto vias = get_vias();
    std::set<UUID> nets;
    auto &brd = *doc.b->get_board();
    for (auto via : vias) {
        if (via->net_set)
            nets.insert(via->net_set->uuid);
    }

    if (tool_id == ToolID::CLEAR_VIA_NET) {
        for (auto via : vias) {
            via->net_set = nullptr;
        }
        brd.expand_flags = Board::EXPAND_PROPAGATE_NETS | Board::EXPAND_AIRWIRES;
        brd.airwires_expand = nets;
        return ToolResponse::commit();
    }

    if (tool_id == ToolID::SET_VIA_NET) {
        if (auto r = imp->dialogs.select_net(*doc.b->get_block(), false)) {
            auto net = &doc.b->get_block()->nets.at(*r);
            nets.insert(net->uuid);
            for (auto via : vias)
                via->net_set = net;
        }
        else {
            return ToolResponse::end();
        }
    }
    brd.expand_flags = Board::EXPAND_PROPAGATE_NETS | Board::EXPAND_AIRWIRES;
    brd.airwires_expand = nets;
    return ToolResponse::commit();
}
ToolResponse ToolSetViaNet::update(const ToolArgs &args)
{
    return ToolResponse();
}
} // namespace horizon
