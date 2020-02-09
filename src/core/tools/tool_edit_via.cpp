#include "tool_edit_via.hpp"
#include "document/idocument_board.hpp"
#include "board/board.hpp"
#include "imp/imp_interface.hpp"
#include <iostream>

namespace horizon {

ToolEditVia::ToolEditVia(IDocument *c, ToolID tid) : ToolBase(c, tid)
{
}

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
    auto holes = get_vias();
    bool r = imp->dialogs.edit_via(holes, *doc.b->get_via_padstack_provider());
    if (r) {
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
