#pragma once
#include <gtkmm.h>

namespace horizon {
class HelpButton : public Gtk::MenuButton {
public:
    enum Flags { FLAG_DEFAULT = 0, FLAG_NON_MODAL = 1 };
    HelpButton(const std::string &markup, Flags flags = FLAG_DEFAULT);
    static void pack_into(Gtk::Box &box, const std::string &markup, Flags flags = FLAG_DEFAULT);
    static void pack_into(const Glib::RefPtr<Gtk::Builder> &x, const char *widget, const std::string &markup,
                          Flags flags = FLAG_DEFAULT);
};

} // namespace horizon
