#include "tool_set_nc.hpp"
#include "document/idocument_schematic.hpp"
#include "schematic/schematic.hpp"
#include "imp/imp_interface.hpp"
#include <iostream>
#include "core/tool_id.hpp"

namespace horizon {

bool ToolSetNotConnected::can_begin()
{
    return doc.c;
}

ToolResponse ToolSetNotConnected::begin(const ToolArgs &args)
{
    if (tool_id == ToolID::CLEAR_NC)
        mode = Mode::CLEAR;
    update_tip();
    imp->tool_bar_set_actions({
            {InToolActionID::LMB, "modify pin"},
            {InToolActionID::RMB, "finish"},
            {InToolActionID::NC_MODE, "mode"},
    });
    return ToolResponse();
}

void ToolSetNotConnected::update_tip()
{
    std::string s;
    switch (mode) {
    case Mode::SET:
        s = "Set";
        break;
    case Mode::CLEAR:
        s = "Clear";
        break;
    case Mode::TOGGLE:
        s = "Toggle";
        break;
    }

    imp->tool_bar_set_tip(s);
}


ToolResponse ToolSetNotConnected::update(const ToolArgs &args)
{
    if (args.type == ToolEventType::ACTION) {
        switch (args.action) {
        case InToolActionID::LMB:
            if (args.target.type == ObjectType::SYMBOL_PIN) {
                auto &sym = doc.c->get_sheet()->symbols.at(args.target.path.at(0));
                auto &pin = sym.symbol.pins.at(args.target.path.at(1));
                UUIDPath<2> connpath(sym.gate->uuid, args.target.path.at(1));
                if (sym.component->connections.count(connpath) == 0) { // pin is not connected
                    if (mode == Mode::TOGGLE || mode == Mode::SET) {
                        sym.component->connections.emplace(connpath, nullptr);
                        sym.symbol.pins.at(pin.uuid).connector_style = SymbolPin::ConnectorStyle::NC; // for preview
                    }
                }
                else {
                    auto &conn = sym.component->connections.at(connpath);
                    if (conn.net == nullptr) {
                        if (mode == Mode::TOGGLE || mode == Mode::CLEAR) {
                            sym.component->connections.erase(connpath);
                            sym.symbol.pins.at(pin.uuid).connector_style =
                                    SymbolPin::ConnectorStyle::BOX; // for preview
                        }
                    }
                    else {
                        imp->tool_bar_flash("Pin is connected to net");
                    }
                }
            }
            else {
                imp->tool_bar_flash("Please click on a pin");
            }
            break;

        case InToolActionID::RMB:
        case InToolActionID::CANCEL:
            return ToolResponse::commit();

        case InToolActionID::NC_MODE:
            switch (mode) {
            case Mode::SET:
                mode = Mode::CLEAR;
                break;

            case Mode::CLEAR:
                mode = Mode::TOGGLE;
                break;

            case Mode::TOGGLE:
                mode = Mode::SET;
                break;
            }
            update_tip();
            break;

        default:;
        }
    }
    return ToolResponse();
}
} // namespace horizon
