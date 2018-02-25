#include "action.hpp"
#include "util/str_util.hpp"
#include <gdkmm.h>

namespace horizon {
std::string key_sequence_to_string(const KeySequence2 &keys)
{
    std::string txt;
    for (const auto &it : keys) {
        std::string keyname(gdk_keyval_name(it.first));
        auto mod = it.second;
        if (mod & Gdk::CONTROL_MASK) {
            txt += "Ctrl-";
        }
        if (mod & Gdk::SHIFT_MASK) {
            txt += "Shift-";
        }
        if (mod & Gdk::MOD1_MASK) {
            txt += "Alt-";
        }
        txt += keyname;
        txt += " ";
    }
    rtrim(txt);
    return txt;
}

std::pair<ActionID, ToolID> make_action(ActionID id)
{
    return {id, ToolID::NONE};
}

std::pair<ActionID, ToolID> make_action(ToolID id)
{
    return {ActionID::TOOL, id};
}

} // namespace horizon
