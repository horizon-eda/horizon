#include "tool_set_group.hpp"
#include "document/idocument_schematic.hpp"
#include "schematic/schematic.hpp"
#include "imp/imp_interface.hpp"
#include <iostream>
#include "core/tool_id.hpp"

namespace horizon {

ToolSetGroup::ToolSetGroup(IDocument *c, ToolID tid) : ToolBase(c, tid)
{
}

std::set<Component *> ToolSetGroup::get_components()
{
    std::set<Component *> components;
    for (const auto &it : selection) {
        if (it.type == ObjectType::SCHEMATIC_SYMBOL) {
            components.insert(doc.c->get_schematic_symbol(it.uuid)->component);
        }
    }
    return components;
}

bool ToolSetGroup::can_begin()
{
    if (!doc.c)
        return false;
    auto comps = get_components();
    auto block = doc.c->get_block();
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
    case ToolID::RENAME_TAG:
        return comps.size() == 1 && ((*comps.begin())->tag);
    default:
        return false;
    }
}

ToolResponse ToolSetGroup::begin(const ToolArgs &args)
{
    auto comps = get_components();
    auto &block = *doc.c->get_block();
    switch (tool_id) {
    case ToolID::SET_GROUP:
    case ToolID::SET_NEW_GROUP:
    case ToolID::CLEAR_GROUP: {
        UUID new_group;
        if (tool_id == ToolID::SET_NEW_GROUP) {
            new_group = UUID::random();
            if (auto r = imp->dialogs.ask_datum_string("Group name", "")) {
                block.group_names[new_group] = *r;
            }
            else {
                return ToolResponse::end();
            }
        }
        else if (tool_id == ToolID::SET_GROUP) {
            if (auto r = imp->dialogs.select_group_tag(block, false, (*comps.begin())->group)) {
                new_group = *r;
            }
            else {
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
            if (auto r = imp->dialogs.ask_datum_string("Tag name", "")) {
                block.tag_names[new_tag] = *r;
            }
            else {
                return ToolResponse::end();
            }
        }
        else if (tool_id == ToolID::SET_TAG) {
            if (auto r = imp->dialogs.select_group_tag(block, true, (*comps.begin())->group)) {
                new_tag = *r;
            }
            else {
                return ToolResponse::end();
            }
        }
        for (auto comp : comps) {
            comp->tag = new_tag;
        }
    } break;

    case ToolID::RENAME_GROUP: {
        auto comp = *comps.begin();
        if (auto r = imp->dialogs.ask_datum_string("Group name", block.get_group_name(comp->group))) {
            if (comp->group)
                block.group_names[comp->group] = *r;
        }
        else {
            return ToolResponse::end();
        }
    } break;

    case ToolID::RENAME_TAG: {
        auto comp = *comps.begin();
        if (auto r = imp->dialogs.ask_datum_string("Tag name", block.get_tag_name(comp->tag))) {
            if (comp->tag)
                block.tag_names[comp->tag] = *r;
        }
        else {
            return ToolResponse::end();
        }
    } break;

    default:;
    }


    return ToolResponse::commit();
}
ToolResponse ToolSetGroup::update(const ToolArgs &args)
{
    return ToolResponse();
}
} // namespace horizon
