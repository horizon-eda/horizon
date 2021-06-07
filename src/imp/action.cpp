#include "action.hpp"
#include "actions.hpp"
#include "util/str_util.hpp"
#include <gdkmm.h>
#include "core/tool_id.hpp"

namespace horizon {
std::string key_sequence_to_string(const KeySequence &keys)
{
    std::string txt;
    for (const auto &it : keys) {
        std::string keyname(gdk_keyval_name(it.first));
        auto mod = it.second;
        if (mod & Gdk::CONTROL_MASK) {
            txt += "Ctrl+";
        }
        if (mod & Gdk::SHIFT_MASK) {
            txt += "Shift+";
        }
        if (mod & Gdk::MOD1_MASK) {
            txt += "Alt+";
        }
        txt += keyname;
        txt += " ";
    }
    rtrim(txt);
    return txt;
}

static std::string keyval_to_string(unsigned int kv)
{
    switch (kv) {
    case GDK_KEY_slash:
        return "/";
    case GDK_KEY_Return:
        return "⏎";
    case GDK_KEY_space:
        return "␣";
    case GDK_KEY_plus:
        return "+";
    case GDK_KEY_minus:
        return "−";
    case GDK_KEY_comma:
        return ",";
    case GDK_KEY_period:
        return ".";
    case GDK_KEY_less:
        return "<";
    case GDK_KEY_greater:
        return ">";
    default:
        return gdk_keyval_name(kv);
    }
}

std::string key_sequence_to_string_short(const KeySequence &keys)
{
    if (keys.size() == 1 && keys.front().second == static_cast<GdkModifierType>(0)) {
        return keyval_to_string(keys.front().first);
    }
    else {
        return key_sequence_to_string(keys);
    }
}

std::string key_sequences_to_string(const std::vector<KeySequence> &seqs)
{
    std::string s;
    for (const auto &it : seqs) {
        if (s.size()) {
            s += ", ";
        }
        s += key_sequence_to_string(it);
    }
    return s;
}

ActionToolID make_action(ActionID id)
{
    return {id, ToolID::NONE};
}

ActionToolID make_action(ToolID id)
{
    return {ActionID::TOOL, id};
}

} // namespace horizon
