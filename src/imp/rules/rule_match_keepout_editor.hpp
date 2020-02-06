#pragma once
#include <gtkmm.h>

namespace horizon {
class RuleMatchKeepoutEditor : public Gtk::Box {
public:
    RuleMatchKeepoutEditor(class RuleMatchKeepout *ma, class IDocument *c);
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
    class IDocument *core;
    type_signal_updated s_signal_updated;
    class ComponentButton *component_button = nullptr;
};
} // namespace horizon
