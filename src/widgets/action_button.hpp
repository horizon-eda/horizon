#pragma once
#include <gtkmm.h>
#include "imp/action.hpp"

namespace horizon {
class ActionButton : public Gtk::Overlay {
public:
    ActionButton(ActionToolID action, const std::map<ActionToolID, std::string> &k);

    typedef sigc::signal<void, ActionToolID> type_signal_clicked;
    type_signal_clicked signal_clicked()
    {
        return s_signal_clicked;
    }

    void update_key_sequences();
    void add_action(ActionToolID act);

private:
    ActionToolID action;
    Gtk::Button *button = nullptr;
    Gtk::MenuButton *menu_button = nullptr;
    Gtk::Menu menu;
    int button_current = -1;
    type_signal_clicked s_signal_clicked;
    Gtk::MenuItem &add_menu_item(ActionToolID act);
    const std::map<ActionToolID, std::string> &keys;
    void set_primary_action(ActionToolID act);
};
} // namespace horizon
