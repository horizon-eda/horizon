#pragma once
#include <set>
#include <gtkmm.h>
#include "util/uuid.hpp"
#include "util/window_state_store.hpp"

namespace horizon {

class TuningWindow : public Gtk::Window {
public:
    TuningWindow(const class Board *brd);
    void update();
    void add_tracks(const std::set<UUID> &tracks, bool all);

private:
    const Board *board;

    class ListColumns : public Gtk::TreeModelColumnRecord {
    public:
        ListColumns()
        {
            Gtk::TreeModelColumnRecord::add(net);
            Gtk::TreeModelColumnRecord::add(net_name);
            Gtk::TreeModelColumnRecord::add(all_tracks);
            Gtk::TreeModelColumnRecord::add(ref);
            Gtk::TreeModelColumnRecord::add(tracks);
            Gtk::TreeModelColumnRecord::add(length);
            Gtk::TreeModelColumnRecord::add(length_ps);
            Gtk::TreeModelColumnRecord::add(delta_ps);
            Gtk::TreeModelColumnRecord::add(fill_value);
        }
        Gtk::TreeModelColumn<UUID> net;
        Gtk::TreeModelColumn<Glib::ustring> net_name;
        Gtk::TreeModelColumn<std::set<UUID>> tracks;
        Gtk::TreeModelColumn<uint64_t> length;
        Gtk::TreeModelColumn<int> fill_value;
        Gtk::TreeModelColumn<double> length_ps;
        Gtk::TreeModelColumn<double> delta_ps;
        Gtk::TreeModelColumn<bool> all_tracks;
        Gtk::TreeModelColumn<bool> ref;
    };
    ListColumns list_columns;

    Glib::RefPtr<Gtk::ListStore> store;
    Gtk::TreeView *tree_view = nullptr;
    Gtk::ScrolledWindow *sc = nullptr;
    Gtk::SpinButton *sp_vf = nullptr;
    Gtk::SpinButton *sp_er = nullptr;

    WindowStateStore state_store;
    Gtk::Label *placeholder_label = nullptr;
};
} // namespace horizon
