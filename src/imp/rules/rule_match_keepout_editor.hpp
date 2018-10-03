#pragma once
#include <gtkmm.h>

namespace horizon {
class RuleMatchKeepoutEditor : public Gtk::Box {
public:
    RuleMatchKeepoutEditor(class RuleMatchKeepout *ma, class Core *c);
    typedef sigc::signal<void> type_signal_updated;
    type_signal_updated signal_updated()
    {
        return s_signal_updated;
    }

private:
    Gtk::ComboBoxText *combo_mode = nullptr;
    Gtk::Stack *sel_stack = nullptr;
    Gtk::Entry *keepout_class_entry = nullptr;
    RuleMatchKeepout *match;
    class Core *core;
    type_signal_updated s_signal_updated;
};
} // namespace horizon
