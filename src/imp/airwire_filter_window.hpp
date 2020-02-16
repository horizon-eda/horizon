#pragma once
#include <gtkmm.h>
#include "util/uuid.hpp"

namespace horizon {

class AirwireFilterWindow : public Gtk::Window {
public:
    static AirwireFilterWindow *create(Gtk::Window *p, const class Block &b, class AirwireFilter &filter);
    AirwireFilterWindow(BaseObjectType *cobject, const Glib::RefPtr<Gtk::Builder> &x, const class Block &b,
                        class AirwireFilter &filter);

    void update_from_filter();

private:
    const class Block &block;
    class AirwireFilter &filter;

    class ListColumns : public Gtk::TreeModelColumnRecord {
    public:
        ListColumns()
        {
            Gtk::TreeModelColumnRecord::add(net);
            Gtk::TreeModelColumnRecord::add(net_name);
            Gtk::TreeModelColumnRecord::add(visible);
            Gtk::TreeModelColumnRecord::add(n_airwires);
        }
        Gtk::TreeModelColumn<UUID> net;
        Gtk::TreeModelColumn<Glib::ustring> net_name;
        Gtk::TreeModelColumn<bool> visible;
        Gtk::TreeModelColumn<unsigned int> n_airwires;
    };
    ListColumns list_columns;

    Glib::RefPtr<Gtk::ListStore> store;
    Gtk::TreeView *treeview = nullptr;
    Gtk::Button *all_on_button = nullptr;
    Gtk::Button *all_off_button = nullptr;
    Gtk::Box *placeholder_box = nullptr;
};

} // namespace horizon
