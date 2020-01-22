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
    auto comps = get_components();
    auto block = core.c->get_block();
    switch (tool_id) {
    case ToolID::SET_GROUP:
        return comps.size() > 0 && block->group_names.size();
    case ToolID::SET_NEW_GROUP:
        return comps.size() > 0;
    case ToolID::CLEAR_GROUP:
        return std::count_if(comps.begin(), comps.end(), [](auto a) { return a->group; }) > 0;
    case ToolID::SET_TAG:
        return comps.size() > 0 && block->tag_names.size();
    case ToolID::SET_NEW_TAG:
        return comps.size() > 0;
    case ToolID::CLEAR_TAG:
        return std::count_if(comps.begin(), comps.end(), [](auto a) { return a->tag; }) > 0;
    case ToolID::RENAME_GROUP:
        return comps.size() == 1 && ((*comps.begin())->group);
    case ToolID::RETAG_GROUP:
        return comps.size() == 1 && ((*comps.begin())->group);
    case ToolID::RENAME_TAG:
        return comps.size() == 1 && ((*comps.begin())->tag);
    default:
        return false;
    }
}

ToolResponse ToolSetGroup::begin(const ToolArgs &args)
{
    auto comps = get_components();
    auto block = core.c->get_block();
    switch (tool_id) {
    case ToolID::SET_GROUP:
    case ToolID::SET_NEW_GROUP:
    case ToolID::CLEAR_GROUP: {
        UUID new_group;
        if (tool_id == ToolID::SET_NEW_GROUP) {
            new_group = UUID::random();
            auto r = imp->dialogs.ask_datum_string("Group name", "");
            if (r.first) {
                block->group_names[new_group] = r.second;
            }
            else {
                core.r->revert();
                return ToolResponse::end();
            }
        }
        else if (tool_id == ToolID::SET_GROUP) {
            auto r = imp->dialogs.select_group_tag(block, false, (*comps.begin())->group);
            if (r.first) {
                new_group = r.second;
            }
            else {
                core.r->revert();
                return ToolResponse::end();
            }
        }
        for (auto comp : comps) {
            comp->group = new_group;
        }
    } break;

    case ToolID::SET_TAG:
    case ToolID::SET_NEW_TAG:
    case ToolID::CLEAR_TAG: {
        UUID new_tag;
        if (tool_id == ToolID::SET_NEW_TAG) {
            new_tag = UUID::random();
            auto r = imp->dialogs.ask_datum_string("Tag name", "");
            if (r.first) {
                block->tag_names[new_tag] = r.second;
            }
            else {
                core.r->revert();
                return ToolResponse::end();
            }
        }
        else if (tool_id == ToolID::SET_TAG) {
            auto r = imp->dialogs.select_group_tag(block, true, (*comps.begin())->group);
            if (r.first) {
                new_tag = r.second;
            }
            else {
                core.r->revert();
                return ToolResponse::end();
            }
        }
        for (auto comp : comps) {
            comp->tag = new_tag;
        }
    } break;

    case ToolID::RENAME_GROUP: {
        auto comp = *comps.begin();
        auto r = imp->dialogs.ask_datum_string("Group name", block->get_group_name(comp->group));
        if (r.first) {
            if (comp->group)
                block->group_names[comp->group] = r.second;
        }
        else {
            core.r->revert();
            return ToolResponse::end();
        }
    } break;

    case ToolID::RENAME_TAG: {
        auto comp = *comps.begin();
        auto r = imp->dialogs.ask_datum_string("Tag name", block->get_tag_name(comp->tag));
        if (r.first) {
            if (comp->tag)
                block->tag_names[comp->tag] = r.second;
        }
        else {
            core.r->revert();
            return ToolResponse::end();
        }
    } break;

    case ToolID::RETAG_GROUP: {
        auto comp = *comps.begin();
        auto group = comp->group;
        int i = 1;
        for (auto &it : block->components) {
            if (it.second.group == group) {
                auto new_tag = UUID::random();
                it.second.tag = new_tag;
                block->tag_names[new_tag] = std::to_string(i);
                i++;
            }
        }
    } break;

    default:;
    }


    core.r->commit();
    return ToolResponse::end();
}
ToolResponse ToolSetGroup::update(const ToolArgs &args)
{
    return ToolResponse();
}
} // namespace horizon
