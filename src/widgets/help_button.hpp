#pragma once
#include <gtkmm.h>

namespace horizon {
class HelpButton : public Gtk::MenuButton {
public:
    HelpButton(const std::string &markup);
    static void pack_into(Gtk::Box &box, const std::string &markup);
    static void pack_into(const Glib::RefPtr<Gtk::Builder> &x, const char *widget, const std::string &markup);
};

} // namespace horizon
