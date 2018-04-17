#include "tool_set_group.hpp"
#include "core_schematic.hpp"
#include "imp/imp_interface.hpp"
#include <iostream>

namespace horizon {

ToolSetGroup::ToolSetGroup(Core *c, ToolID tid) : ToolBase(c, tid)
{
}

std::set<Component *> ToolSetGroup::get_components()
{
    std::set<Component *> components;
    for (const auto &it : core.r->selection) {
        if (it.type == ObjectType::SCHEMATIC_SYMBOL) {
            components.insert(core.c->get_schematic_symbol(it.uuid)->component);
        }
    }
    return components;
}

bool ToolSetGroup::can_begin()
{
    if (!core.c)
        return false;
    return get_components().size() > 0;
}

ToolResponse ToolSetGroup::begin(const ToolArgs &args)
{
    auto comps = get_components();

    if (tool_id == ToolID::SET_GROUP || tool_id == ToolID::CLEAR_GROUP) {
        UUID new_group;
        if (tool_id == ToolID::SET_GROUP)
            new_group = UUID::random();
        for (auto comp : comps) {
            comp->group = new_group;
        }
    }
    if (tool_id == ToolID::SET_TAG || tool_id == ToolID::CLEAR_TAG) {
        UUID new_tag;
        if (tool_id == ToolID::SET_TAG)
            new_tag = UUID::random();
        for (auto comp : comps) {
            comp->tag = new_tag;
        }
    }

    core.r->commit();
    return ToolResponse::end();
}
ToolResponse ToolSetGroup::update(const ToolArgs &args)
{
    return ToolResponse();
}
} // namespace horizon
