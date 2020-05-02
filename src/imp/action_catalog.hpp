#pragma once
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
        AVAILABLE_IN_FRAME = (1 << 5),
        AVAILABLE_EVERYWHERE = 0xff,
        AVAILABLE_LAYERED = AVAILABLE_IN_PACKAGE | AVAILABLE_IN_PADSTACK | AVAILABLE_IN_BOARD,
        AVAILABLE_IN_PACKAGE_AND_BOARD = AVAILABLE_IN_PACKAGE | AVAILABLE_IN_BOARD,
        AVAILABLE_IN_SCHEMATIC_AND_BOARD = AVAILABLE_IN_SCHEMATIC | AVAILABLE_IN_BOARD
    };

    enum Flags {
        FLAGS_DEFAULT = 0,
        FLAGS_IN_TOOL = (1 << 1),
        FLAGS_NO_POPOVER = (1 << 2),
        FLAGS_NO_MENU = (1 << 3),
        FLAGS_SPECIFIC = (1 << 4),
        FLAGS_NO_PREFERENCES = (1 << 5),
    };

    ActionCatalogItem(const std::string &n, ActionGroup gr, int av, int fl = FLAGS_DEFAULT)
        : name(n), group(gr), flags(static_cast<Flags>(fl)), availability(static_cast<Availability>(av)){};

    const std::string name;
    ActionGroup group;
    const Flags flags;
    const Availability availability;
};

extern const std::map<ActionToolID, ActionCatalogItem> action_catalog;
extern const LutEnumStr<ActionID> action_lut;
extern const LutEnumStr<ToolID> tool_lut;

extern const std::vector<std::pair<ActionGroup, std::string>> action_group_catalog;
} // namespace horizon
