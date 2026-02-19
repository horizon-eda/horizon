#pragma once
#include <gtkmm.h>
#include "util/changeable.hpp"

namespace horizon {
class TextEditor : public Gtk::Stack, public Changeable {
public:
    enum class Lines { MULTI, SINGLE };
    enum class ShowHints { YES, NO };
    TextEditor(Lines mode = Lines::SINGLE, ShowHints show_hint = ShowHints::YES);
    enum class Select { YES, NO };
    void set_text(const std::string &text, Select select);
    std::string get_text() const;
    bool is_multi_line() const;

    type_signal_changed signal_activate()
    {
        return s_signal_activate;
    }

    type_signal_changed signal_lost_focus()
    {
        return s_signal_lost_focus;
    }

private:
    Gtk::Entry *entry = nullptr;
    Gtk::TextView *view = nullptr;

    type_signal_changed s_signal_activate;
    type_signal_changed s_signal_lost_focus;
    sigc::connection entry_focus_out_conn;
};
} // namespace horizon
