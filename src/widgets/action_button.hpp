#pragma once
#include <gtkmm.h>
#include "imp/action.hpp"

namespace horizon {
class ActionButton : public Gtk::Overlay {
public:
    ActionButton(ActionToolID action, const char *icon_name);

    typedef sigc::signal<void, ActionToolID> type_signal_clicked;
    type_signal_clicked signal_clicked()
    {
        return s_signal_clicked;
    }

    void set_key_sequences(const std::string &keys);

private:
    ActionToolID action;
    Gtk::Button *button = nullptr;
    type_signal_clicked s_signal_clicked;
};
} // namespace horizon
