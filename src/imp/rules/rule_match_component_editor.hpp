#pragma once
#include <gtkmm.h>

namespace horizon {
class RuleMatchComponentEditor : public Gtk::Box {
public:
    RuleMatchComponentEditor(class RuleMatchComponent &ma, class IDocument &c);
    typedef sigc::signal<void> type_signal_updated;
    type_signal_updated signal_updated()
    {
        return s_signal_updated;
    }

private:
    Gtk::ComboBoxText *combo_mode = nullptr;
    Gtk::Stack *sel_stack = nullptr;
    class ComponentButton *component_button = nullptr;
    class MultiComponentButton *multi_component_button = nullptr;
    class PoolBrowserButton *part_button = nullptr;
    RuleMatchComponent &match;
    class IDocument &core;
    type_signal_updated s_signal_updated;
};
} // namespace horizon
