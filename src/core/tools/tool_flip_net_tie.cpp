#include "tool_flip_net_tie.hpp"
#include "document/idocument_schematic.hpp"
#include "schematic/schematic.hpp"
#include "util/selection_util.hpp"

namespace horizon {

bool ToolFlipNetTie::can_begin()
{
    return sel_find_exactly_one(selection, ObjectType::SCHEMATIC_NET_TIE);
}

ToolResponse ToolFlipNetTie::begin(const ToolArgs &args)
{
    auto &s = sel_find_one(selection, ObjectType::SCHEMATIC_NET_TIE);
    auto &tie = doc.c->get_sheet()->net_ties.at(s.uuid).net_tie;
    std::swap(tie->net_primary, tie->net_secondary);
    return ToolResponse::commit();
}
ToolResponse ToolFlipNetTie::update(const ToolArgs &args)
{
    return ToolResponse();
}
} // namespace horizon
