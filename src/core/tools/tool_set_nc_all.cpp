#include "tool_set_nc_all.hpp"
#include "document/idocument_schematic.hpp"
#include "schematic/schematic.hpp"
#include <iostream>
#include "core/tool_id.hpp"

namespace horizon {

ToolSetNotConnectedAll::ToolSetNotConnectedAll(IDocument *c, ToolID tid) : ToolBase(c, tid)
{
}

bool ToolSetNotConnectedAll::can_begin()
{
    for (const auto &it : selection) {
        if (it.type == ObjectType::SCHEMATIC_SYMBOL) {
            auto sym = doc.c->get_schematic_symbol(it.uuid);
            auto gate = sym->gate;
            for (auto &it_pin : gate->unit->pins) {
                auto path = UUIDPath<2>(gate->uuid, it_pin.first);
                if (tool_id == ToolID::SET_ALL_NC) {
                    if (sym->component->connections.count(path) == 0) { // unconnected pin
                        return true;
                    }
                }
                else if (tool_id == ToolID::CLEAR_ALL_NC) {
                    if (sym->component->connections.count(path)
                        && sym->component->connections.at(path).net == nullptr) { // nc pin
                        return true;
                    }
                }
            }
        }
    }
    return false;
}

ToolResponse ToolSetNotConnectedAll::begin(const ToolArgs &args)
{
    for (const auto &it : args.selection) {
        if (it.type == ObjectType::SCHEMATIC_SYMBOL) {
            auto sym = doc.c->get_schematic_symbol(it.uuid);
            auto gate = sym->gate;
            for (auto &it_pin : gate->unit->pins) {
                auto path = UUIDPath<2>(gate->uuid, it_pin.first);
                if (tool_id == ToolID::SET_ALL_NC) {
                    if (!sym->component->connections.count(path)) {
                        sym->component->connections.emplace(path, nullptr);
                    }
                }
                else {
                    if (sym->component->connections.count(path)
                        && sym->component->connections.at(path).net == nullptr) {
                        sym->component->connections.erase(path);
                    }
                }
            }
        }
    }
    return ToolResponse::commit();
}
ToolResponse ToolSetNotConnectedAll::update(const ToolArgs &args)
{
    return ToolResponse();
}
} // namespace horizon
