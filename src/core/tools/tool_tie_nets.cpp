#include "tool_tie_nets.hpp"
#include "document/idocument_schematic.hpp"
#include "schematic/schematic.hpp"
#include "imp/imp_interface.hpp"
#include "util/selection_util.hpp"

namespace horizon {

bool ToolTieNets::can_begin()
{
    if (!doc.c) {
        return false;
    }

    auto junctions = get_junctions();
    return junctions.first && junctions.second;
}

std::pair<SchematicJunction *, SchematicJunction *> ToolTieNets::get_junctions()
{
    if (sel_count_type(selection, ObjectType::JUNCTION) != 2)
        return {nullptr, nullptr};
    std::vector<SchematicJunction *> junctions;
    for (const auto &it : selection) {
        if (it.type == ObjectType::JUNCTION)
            junctions.push_back(&doc.c->get_sheet()->junctions.at(it.uuid));
    }

    auto &j1 = *junctions.at(0);
    auto &j2 = *junctions.at(1);

    if (!j1.net)
        return {nullptr, nullptr};
    if (!j2.net)
        return {nullptr, nullptr};
    if (j1.net == j2.net)
        return {nullptr, nullptr};

    return {&j1, &j2};
}


ToolResponse ToolTieNets::begin(const ToolArgs &args)
{
    auto junctions = get_junctions();
    auto &block = *doc.c->get_current_block();
    const auto uu_tie = UUID::random();
    auto &tie = block.net_ties.emplace(uu_tie, uu_tie).first->second;
    tie.net_primary = junctions.first->net;
    tie.net_secondary = junctions.second->net;

    const auto uu_stie = UUID::random();
    auto &stie = doc.c->get_sheet()->net_ties.emplace(uu_stie, uu_stie).first->second;
    stie.net_tie = &tie;
    stie.from = junctions.first;
    stie.to = junctions.second;

    return ToolResponse::commit();
}

ToolResponse ToolTieNets::update(const ToolArgs &args)
{
    return ToolResponse();
}
} // namespace horizon
