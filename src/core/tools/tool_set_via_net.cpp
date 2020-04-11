#include "tool_set_via_net.hpp"
#include "document/idocument_board.hpp"
#include "board/board.hpp"
#include "imp/imp_interface.hpp"
#include <iostream>
#include "core/tool_id.hpp"

namespace horizon {

ToolSetViaNet::ToolSetViaNet(IDocument *c, ToolID tid) : ToolBase(c, tid)
{
}

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

    if (tool_id == ToolID::CLEAR_VIA_NET) {
        for (auto via : vias)
            via->net_set = nullptr;
        return ToolResponse::commit();
    }

    if (tool_id == ToolID::SET_VIA_NET) {
        auto r = imp->dialogs.select_net(doc.b->get_block(), false);
        if (r.first) {
            auto net = &doc.b->get_block()->nets.at(r.second);
            for (auto via : vias)
                via->net_set = net;
        }
    }
    return ToolResponse::commit();
}
ToolResponse ToolSetViaNet::update(const ToolArgs &args)
{
    return ToolResponse();
}
} // namespace horizon
