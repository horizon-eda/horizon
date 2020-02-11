#pragma once
#include <gtkmm.h>

namespace horizon {
class HeaderButton : public Gtk::MenuButton {
public:
    HeaderButton();
    void set_label(const std::string &l);
    Gtk::Entry *add_entry(const std::string &label);
    void add_widget(const std::string &label, Gtk::Widget *w);

    typedef sigc::signal<void> type_signal_closed;
    type_signal_closed signal_closed()
    {
        return s_signal_closed;
    }

private:
    int top = 0;
    Gtk::Label *label = nullptr;
    Gtk::Grid *grid = nullptr;
    type_signal_closed s_signal_closed;
};
} // namespace horizon
