#pragma once
#include <string>
#include <vector>
#include <gdk/gdk.h>
#include <functional>

namespace horizon {

enum class ToolID;
enum class ActionID;


struct ActionToolID {
    ActionToolID(ActionID a, ToolID t) : action(a), tool(t)
    {
    }

    ActionToolID(ActionID a);
    ActionToolID(ToolID t);
    ActionToolID();

    bool is_tool() const;
    bool is_action() const;
    bool is_valid() const;

    ActionID action;
    ToolID tool;

private:
    auto tie() const
    {
        return std::tie(action, tool);
    }

public:
    bool operator<(const ActionToolID &other) const
    {
        return tie() < other.tie();
    }
    bool operator==(const ActionToolID &other) const
    {
        return tie() == other.tie();
    }
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
    RULES,
    VIEW,
    FRAME,
    GROUP_TAG,
    SEARCH,
    EXPORT_IMPORT,
    TUNING,
    VIEW_3D,
};

using KeySequenceItem = std::pair<unsigned int, GdkModifierType>;
using KeySequence = std::vector<KeySequenceItem>;

std::string key_sequence_item_to_string(const KeySequenceItem &it);
std::string key_sequence_to_string(const KeySequence &keys);
std::string key_sequence_to_string_short(const KeySequence &keys);

std::string key_sequences_to_string(const std::vector<KeySequence> &seqs);

enum class KeyMatchResult { NONE, PREFIX, COMPLETE };
KeyMatchResult key_sequence_match(const KeySequence &keys_current, const KeySequence &keys_from_action);

class ActionConnection {
public:
    ActionConnection(ActionToolID atid, std::function<void(const ActionConnection &)> c) : id(atid), cb(c)
    {
    }

    const ActionToolID id;
    std::vector<KeySequence> key_sequences;
    std::function<void(const ActionConnection &)> cb;
};

} // namespace horizon
