#pragma once
#include <string>
#include <vector>
#include <gdk/gdk.h>
#include <functional>

namespace horizon {

enum class ToolID;

enum class ActionID {
    NONE,
    TOOL,
    UNDO,
    REDO,
    COPY,
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
    FLIP_VIEW,
    VIEW_TOP,
    VIEW_BOTTOM,
    EDIT_PADSTACK,
    EDIT_UNIT,
    HIGHLIGHT_GROUP,
    HIGHLIGHT_TAG,
    SELECT_GROUP,
    SELECT_TAG,
    SEARCH,
    SEARCH_NEXT,
    SEARCH_PREVIOUS,
    GO_TO_BOARD,
    GO_TO_SCHEMATIC,
    SHOW_IN_POOL_MANAGER,
    SELECT_ALL,
    PDF_EXPORT_WINDOW,
    EXPORT_PDF,
    BACKANNOTATE_CONNECTION_LINES,
    SELECT_MORE,
    SELECT_MORE_NO_VIA,
    STEP_EXPORT_WINDOW,
    EXPORT_STEP,
    PNP_EXPORT_WINDOW,
    EXPORT_PNP,
    RESET_AIRWIRE_FILTER,
    FILTER_AIRWIRES,
    AIRWIRE_FILTER_WINDOW,
    SELECT_POLYGON,
    FAB_OUTPUT_WINDOW,
    GEN_FAB_OUTPUT,
    FOOTPRINT_GENERATOR
};

using ActionToolID = std::pair<ActionID, ToolID>;

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
    RULES,
    VIEW,
    FRAME,
    GROUP_TAG,
    SEARCH,
    EXPORT_IMPORT,
    TUNING,
};

typedef std::vector<std::pair<unsigned int, GdkModifierType>> KeySequence;

std::string key_sequence_to_string(const KeySequence &keys);
std::string key_sequences_to_string(const std::vector<KeySequence> &seqs);

class ActionConnection {
public:
    ActionConnection(ActionID aid, ToolID tid, std::function<void(const ActionConnection &)> c)
        : action_id(aid), tool_id(tid), cb(c)
    {
    }

    const ActionID action_id;
    const ToolID tool_id;
    std::vector<KeySequence> key_sequences;
    std::function<void(const ActionConnection &)> cb;
};

ActionToolID make_action(ActionID id);
ActionToolID make_action(ToolID id);

} // namespace horizon
