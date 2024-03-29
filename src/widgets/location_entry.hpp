#pragma once
#include <gtkmm.h>

namespace horizon {

class LocationEntry : public Gtk::Box {
public:
    LocationEntry(const std::string &rel = "");
    void set_filename(const std::string &s);
    void set_rel_filename(const std::string &s);
    void set_relative_to(const std::string &s);
    std::string get_filename();
    void set_warning(const std::string &t);

    typedef sigc::signal<void> type_signal_changed;
    type_signal_changed signal_changed()
    {
        return s_signal_changed;
    }

    bool check_ends_json();
    void set_append_json(bool s);

private:
    std::string relative_to;
    std::string get_rel_filename(const std::string &s) const;
    Gtk::Entry *entry = nullptr;
    bool autoappend_json = false;
    void handle_button();

    type_signal_changed s_signal_changed;
};
} // namespace horizon
