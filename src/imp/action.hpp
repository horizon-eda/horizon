#pragma once
#include <string>
#include <vector>
#include <gdk/gdk.h>
#include "core/core.hpp"

namespace horizon {

enum class ActionID {
    NONE,
    TOOL,
    UNDO,
    REDO,
    COPY,
    DUPLICATE,
    PLACE_PART,
    LAYER_UP,
    LAYER_DOWN,
    LAYER_TOP,
    LAYER_BOTTOM,
    LAYER_INNER1,
    LAYER_INNER2,
    LAYER_INNER3,
    LAYER_INNER4,
    LAYER_INNER5,
    LAYER_INNER6,
    LAYER_INNER7,
    LAYER_INNER8,
    SELECTION_FILTER,
    SAVE,
    VIEW_3D,
    RULES,
    RULES_RUN_CHECKS,
    RULES_APPLY,
    PREFERENCES,
    POPOVER,
    HELP,
    VIEW_ALL,
    SELECTION_TOOL_BOX,
    SELECTION_TOOL_LASSO,
    SELECTION_TOOL_PAINT,
    SELECTION_QUALIFIER_AUTO,
    SELECTION_QUALIFIER_INCLUDE_ORIGIN,
    SELECTION_QUALIFIER_TOUCH_BOX,
    SELECTION_QUALIFIER_INCLUDE_BOX,
    TO_BOARD,
    MOVE_TO_OTHER_SHEET,
    SHOW_IN_BROWSER,
    TUNING,
    TUNING_ADD_TRACKS,
    TUNING_ADD_TRACKS_ALL,
    HIGHLIGHT_NET,
    RELOAD_NETLIST,
    SAVE_RELOAD_NETLIST,
    BOM_EXPORT_WINDOW,
    EXPORT_BOM,
    RELOAD_POOL,
};

enum class ActionGroup {
    ALL,
    UNKNOWN,
    CLIPBOARD,
    UNDO,
    MOVE,
    GRAPHICS,
    SCHEMATIC,
    SYMBOL,
    PACKAGE,
    PADSTACK,
    BOARD,
    LAYER,
    SELECTION,
    RULES
};

typedef std::vector<std::pair<int, GdkModifierType>> KeySequence2;

std::string key_sequence_to_string(const KeySequence2 &keys);

class ActionConnection {
public:
    ActionConnection(ActionID aid, ToolID tid, std::function<void(const ActionConnection &)> c)
        : action_id(aid), tool_id(tid), cb(c)
    {
    }

    const ActionID action_id;
    const ToolID tool_id;
    std::vector<KeySequence2> key_sequences;
    std::function<void(const ActionConnection &)> cb;
};

std::pair<ActionID, ToolID> make_action(ActionID id);
std::pair<ActionID, ToolID> make_action(ToolID id);

} // namespace horizon
