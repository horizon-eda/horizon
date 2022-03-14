#pragma once
#include <gtkmm.h>

namespace horizon {
class DoneRevealerController {
public:
    void attach(Gtk::Revealer *rev, Gtk::Label *la, Gtk::Button *bu);
    void show_success(const std::string &s);
    void show_error(const std::string &s);

private:
    Gtk::Revealer *revealer = nullptr;
    Gtk::Label *label = nullptr;
    Gtk::Button *button = nullptr;

    sigc::connection flash_connection;
};
} // namespace horizon
