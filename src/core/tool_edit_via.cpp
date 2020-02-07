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
    if (!doc.b)
        return false;
    return std::count_if(selection.begin(), selection.end(), [](const auto &x) { return x.type == ObjectType::VIA; })
           == 1;
}

ToolResponse ToolEditVia::begin(const ToolArgs &args)
{
    std::cout << "tool edit via\n";
    auto board = doc.b->get_board();

    auto uu = std::find_if(selection.begin(), selection.end(), [](const auto &x) {
                  return x.type == ObjectType::VIA;
              })->uuid;
    auto via = &board->vias.at(uu);

    bool r = imp->dialogs.edit_via(via, *doc.b->get_via_padstack_provider());
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
