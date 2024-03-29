#pragma once
#include <gtkmm.h>

namespace horizon {
class RuleMatchEditor : public Gtk::Box {
public:
    RuleMatchEditor(class RuleMatch &ma, class IDocument &c);
    typedef sigc::signal<void> type_signal_updated;
    type_signal_updated signal_updated()
    {
        return s_signal_updated;
    }

private:
    Gtk::ComboBoxText *combo_mode = nullptr;
    Gtk::Stack *sel_stack = nullptr;
    class NetButton *net_button = nullptr;
    class MultiNetButton *multi_net_button = nullptr;
    class NetClassButton *net_class_button = nullptr;
    Gtk::Entry *net_name_regex_entry = nullptr;
    Gtk::Entry *net_class_regex_entry = nullptr;
    RuleMatch &match;
    class IDocument &core;
    type_signal_updated s_signal_updated;
};
} // namespace horizon
