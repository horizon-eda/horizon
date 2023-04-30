#include "tool_edit_via.hpp"
#include "document/idocument_board.hpp"
#include "board/board.hpp"
#include "imp/imp_interface.hpp"
#include <iostream>

namespace horizon {

bool ToolEditVia::can_begin()
{
    return get_vias().size() > 0;
}

std::set<Via *> ToolEditVia::get_vias()
{
    std::set<Via *> vias;
    for (const auto &it : selection) {
        if (it.type == ObjectType::VIA) {
            vias.emplace(&doc.b->get_board()->vias.at(it.uuid));
        }
    }
    return vias;
}

ToolResponse ToolEditVia::begin(const ToolArgs &args)
{
    auto vias = get_vias();
    bool r = imp->dialogs.edit_via(vias, doc.b->get_pool(), doc.b->get_pool_caching(), doc.b->get_layer_provider(),
                                   doc.b->get_rules()->get_rule_t<RuleViaDefinitions>());
    if (r) {
        for (auto via : vias) {
            via->expand(*doc.b->get_board());
        }
        return ToolResponse::commit();
    }
    else {
        return ToolResponse::revert();
    }
}
ToolResponse ToolEditVia::update(const ToolArgs &args)
{
    return ToolResponse();
}
} // namespace horizon
