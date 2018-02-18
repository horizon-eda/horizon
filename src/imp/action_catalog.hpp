#pragma once
#include "core/core.hpp"
#include "action.hpp"
#include "common/lut.hpp"
#include <map>

namespace horizon {
class ActionCatalogItem {
public:
    enum Availability {
        AVAILABLE_IN_SYMBOL = (1 << 0),
        AVAILABLE_IN_SCHEMATIC = (1 << 1),
        AVAILABLE_IN_PADSTACK = (1 << 2),
        AVAILABLE_IN_PACKAGE = (1 << 3),
        AVAILABLE_IN_BOARD = (1 << 4),
        AVAILABLE_EVERYWHERE = 0xff,
        AVAILABLE_LAYERED = AVAILABLE_IN_PACKAGE | AVAILABLE_IN_PADSTACK | AVAILABLE_IN_BOARD,
        AVAILABLE_IN_PACKAGE_AND_BOARD = AVAILABLE_IN_PACKAGE | AVAILABLE_IN_BOARD,
        AVAILABLE_IN_SCHEMATIC_AND_BOARD = AVAILABLE_IN_SCHEMATIC | AVAILABLE_IN_BOARD
    };

    ActionCatalogItem(const std::string &n, ActionGroup gr, Availability av, bool i = false)
        : name(n), group(gr), in_tool(i), availability(av){};

    const std::string name;
    ActionGroup group;
    const bool in_tool;
    const Availability availability;
};

extern const std::map<std::pair<ActionID, ToolID>, ActionCatalogItem> action_catalog;
extern const LutEnumStr<ActionID> action_lut;
extern const LutEnumStr<ToolID> tool_lut;
} // namespace horizon
