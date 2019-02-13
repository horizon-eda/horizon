#include "component_button.hpp"

namespace horizon {
ComponentButton::ComponentButton(Block *b) : Gtk::MenuButton(), block(b)
{
    popover = Gtk::manage(new Gtk::Popover(*this));
    cs = Gtk::manage(new ComponentSelector(block));
    cs->set_size_request(100, 200);
    cs->signal_activated().connect(sigc::mem_fun(*this, &ComponentButton::cs_activated));
    cs->show();

    popover->add(*cs);
    set_popover(*popover);
    component_current = cs->get_selected_component();
    update_label();
}

void ComponentButton::on_toggled()
{
    cs->update();
    cs->select_component(component_current);
    Gtk::ToggleButton::on_toggled();
}

void ComponentButton::update()
{
    cs->update();
    on_toggled();
    update_label();
}

void ComponentButton::set_component(const UUID &uu)
{
    if (block->components.count(uu)) {
        component_current = uu;
    }
    else {
        component_current = UUID();
    }
    update_label();
}

UUID ComponentButton::get_component()
{
    return component_current;
}

void ComponentButton::update_label()
{
    if (component_current) {
        set_label(block->components.at(component_current).refdes);
    }
    else {
        set_label("<no component>");
    }
}

void ComponentButton::cs_activated(const UUID &uu)
{
    set_active(false);
    component_current = uu;
    s_signal_changed.emit(uu);
    update_label();
}
} // namespace horizon
