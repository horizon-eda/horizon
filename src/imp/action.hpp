#pragma once
#include <string>
#include <vector>
#include <gdk/gdk.h>
#include <functional>

namespace horizon {

enum class ToolID;
enum class ActionID;

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
std::string key_sequence_to_string_short(const KeySequence &keys);

std::string key_sequences_to_string(const std::vector<KeySequence> &seqs);

enum class KeyMatchResult { NONE, PREFIX, COMPLETE };
KeyMatchResult key_sequence_match(const KeySequence &keys_current, const KeySequence &keys_from_action);

class ActionConnection {
public:
    ActionConnection(ActionToolID id, std::function<void(const ActionConnection &)> c)
        : action_id(id.first), tool_id(id.second), cb(c)
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
