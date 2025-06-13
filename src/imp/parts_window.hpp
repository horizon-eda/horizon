#pragma once
#include <set>
#include <gtkmm.h>
#include "util/uuid.hpp"
#include "util/window_state_store.hpp"
#include <nlohmann/json_fwd.hpp>

namespace horizon {
using json = nlohmann::json;

class PartsWindow : public Gtk::Window {
public:
    PartsWindow(const class Board &brd);
    void update();

    typedef sigc::signal<void, std::set<UUID>> type_signal_selected;
    type_signal_selected signal_selected()
    {
        return s_signal_selected;
    }

    json serialize() const;
    void load_from_json(const json &j);
    void reset_placed();

private:
    const Board &board;

    class ListColumns : public Gtk::TreeModelColumnRecord {
    public:
        ListColumns()
        {
            Gtk::TreeModelColumnRecord::add(package);
            Gtk::TreeModelColumnRecord::add(MPN);
            Gtk::TreeModelColumnRecord::add(value);
            Gtk::TreeModelColumnRecord::add(refdes);
            Gtk::TreeModelColumnRecord::add(qty);
            Gtk::TreeModelColumnRecord::add(components);
            Gtk::TreeModelColumnRecord::add(placed);
            Gtk::TreeModelColumnRecord::add(part);
        }
        Gtk::TreeModelColumn<Glib::ustring> package;
        Gtk::TreeModelColumn<Glib::ustring> MPN;
        Gtk::TreeModelColumn<Glib::ustring> value;
        Gtk::TreeModelColumn<Glib::ustring> refdes;
        Gtk::TreeModelColumn<unsigned int> qty;
        Gtk::TreeModelColumn<std::set<UUID>> components;
        Gtk::TreeModelColumn<bool> placed;
        Gtk::TreeModelColumn<UUID> part;
    };
    ListColumns list_columns;

    Glib::RefPtr<Gtk::ListStore> store;
    Gtk::TreeView *tree_view = nullptr;

    WindowStateStore state_store;

    type_signal_selected s_signal_selected;
};
} // namespace horizon
