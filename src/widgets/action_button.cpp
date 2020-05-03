#include "action_button.hpp"
#include "imp/action_catalog.hpp"
#include <iostream>

namespace horizon {
ActionButton::ActionButton(ActionToolID act, const char *icon_name) : action(act)
{
    get_style_context()->add_class("osd");
    button = Gtk::manage(new Gtk::Button);
    button->set_image_from_icon_name(icon_name, Gtk::ICON_SIZE_DND);
    button->set_tooltip_text(action_catalog.at(action).name);
    button->signal_clicked().connect([this] { s_signal_clicked.emit(action); });
    button->signal_button_press_event().connect(
            [this](GdkEventButton *ev) {
                if (!menu_button->get_visible())
                    return false;
                if (gdk_event_triggers_context_menu((GdkEvent *)ev)) {
                    menu.popup_at_widget(button, Gdk::GRAVITY_NORTH_EAST, Gdk::GRAVITY_NORTH_WEST, (GdkEvent *)ev);
                    return true;
                }
                else if (ev->button == 1) {
                    button_current = 1;
                }
                return false;
            },
            false);
    button->signal_button_release_event().connect([this](GdkEventButton *ev) {
        button_current = -1;
        return false;
    });
    button->signal_leave_notify_event().connect([this](GdkEventCrossing *ev) {
        if (ev->x > button->get_allocated_width() / 2 && button_current == 1) {
            menu.popup_at_widget(button, Gdk::GRAVITY_NORTH_EAST, Gdk::GRAVITY_NORTH_WEST, (GdkEvent *)ev);
        }
        button_current = -1;
        return false;
    });
    add(*button);
    button->show();
    menu_button = Gtk::manage(new Gtk::MenuButton);
    menu_button->set_image_from_icon_name("action-button-expander-symbolic", Gtk::ICON_SIZE_BUTTON);
    menu_button->get_style_context()->add_class("imp-action-bar-expander");
    menu_button->set_relief(Gtk::RELIEF_NONE);
    menu_button->set_halign(Gtk::ALIGN_END);
    menu_button->set_valign(Gtk::ALIGN_END);
    menu_button->set_direction(Gtk::ARROW_RIGHT);
    menu_button->set_menu(menu);
    menu_button->signal_button_press_event().connect(
            [this](GdkEventButton *ev) {
                if (!menu_button->get_visible())
                    return false;
                if (ev->button == 1) {
                    button_current = 1;
                }
                return false;
            },
            false);
    menu_button->signal_button_release_event().connect([this](GdkEventButton *ev) {
        button_current = -1;
        return false;
    });
    menu_button->signal_leave_notify_event().connect([this](GdkEventCrossing *ev) {
        if (ev->x > menu_button->get_allocated_width() / 2 && button_current == 1) {
            menu.popup_at_widget(menu_button, Gdk::GRAVITY_NORTH_EAST, Gdk::GRAVITY_NORTH_WEST, (GdkEvent *)ev);
        }
        button_current = -1;
        return false;
    });
    add_overlay(*menu_button);
    menu_button->set_no_show_all();
}

void ActionButton::set_key_sequences(const std::string &keys)
{
    button->set_tooltip_text(action_catalog.at(action).name + " (" + keys + ")");
}

void ActionButton::add_action(ActionToolID act)
{
    menu_button->show();
    {
        auto it = Gtk::manage(new Gtk::MenuItem(action_catalog.at(act).name));
        it->show();
        it->signal_activate().connect([this, act] { s_signal_clicked.emit(act); });
        menu.append(*it);
    }
}
} // namespace horizon
