#pragma once
#include <gtkmm.h>

namespace horizon {
class RecentItemBox : public Gtk::EventBox {
public:
    RecentItemBox(const std::string &name, const std::string &path, const Glib::DateTime &time);
    const std::string path;
    typedef sigc::signal<void> type_signal_remove;
    type_signal_remove signal_remove()
    {
        return s_signal_remove;
    }

private:
    const Glib::DateTime time;
    void update_time();
    Gtk::Label *time_label = nullptr;
    Gtk::Menu menu;
    type_signal_remove s_signal_remove;
};
} // namespace horizon
