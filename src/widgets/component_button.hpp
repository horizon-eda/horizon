#pragma once
#include <gtkmm.h>
#include "block/block.hpp"
#include "component_selector.hpp"

namespace horizon {

class ComponentButton : public Gtk::MenuButton {
public:
    ComponentButton(Block *b);
    void set_component(const UUID &uu);
    UUID get_component();
    typedef sigc::signal<void, UUID> type_signal_changed;
    type_signal_changed signal_changed()
    {
        return s_signal_changed;
    }
    void update();

    void set_no_expand(bool e);

private:
    Block *block;
    Gtk::Popover *popover;
    ComponentSelector *cs;
    void update_label();
    void cs_activated(const UUID &uu);
    UUID component_current;
    void on_toggled() override;

    Gtk::Label *label = nullptr;


    type_signal_changed s_signal_changed;
};
} // namespace horizon
