#include "action_button.hpp"
#include "imp/action_catalog.hpp"

namespace horizon {
ActionButton::ActionButton(ActionToolID act, const char *icon_name) : action(act)
{
    button = Gtk::manage(new Gtk::Button);
    button->set_image_from_icon_name(icon_name, Gtk::ICON_SIZE_DND);
    button->set_tooltip_text(action_catalog.at(action).name);
    button->signal_clicked().connect([this] { s_signal_clicked.emit(action); });
    add(*button);
    button->show();
    /*auto ex = Gtk::manage(new Gtk::Button);
    ex->set_image_from_icon_name("action-button-expander-symbolic", Gtk::ICON_SIZE_BUTTON);
    ex->get_style_context()->add_class("imp-action-bar-expander");
    ex->set_relief(Gtk::RELIEF_NONE);
    ex->set_halign(Gtk::ALIGN_END);
    ex->set_valign(Gtk::ALIGN_END);
    add_overlay(*ex);
    ex->show();*/
}

void ActionButton::set_key_sequences(const std::string &keys)
{
    button->set_tooltip_text(action_catalog.at(action).name + " " + keys);
}
} // namespace horizon
