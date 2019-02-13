#pragma once
#include <gtkmm.h>
#include <memory>
#include <zmq.hpp>

namespace horizon {

class PoolProjectManagerViewCreatePool {
public:
    PoolProjectManagerViewCreatePool(const Glib::RefPtr<Gtk::Builder> &refBuilder,
                                     class PoolProjectManagerAppWindow *w);
    void clear();
    std::pair<bool, std::string> create();
    typedef sigc::signal<void, bool> type_signal_valid_change;
    type_signal_valid_change signal_valid_change()
    {
        return s_signal_valid_change;
    }
    void update();

private:
    Gtk::Entry *pool_name_entry = nullptr;
    Gtk::FileChooserButton *pool_path_chooser = nullptr;

    type_signal_valid_change s_signal_valid_change;
};
} // namespace horizon
