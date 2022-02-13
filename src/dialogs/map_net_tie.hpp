#pragma once
#include <gtkmm.h>
#include <set>
#include "common/common.hpp"
#include "util/uuid.hpp"
namespace horizon {


class MapNetTieDialog : public Gtk::Dialog {
public:
    MapNetTieDialog(Gtk::Window *parent, const std::set<class NetTie *> &net_ties);
    UUID selected_uuid;
    bool selection_valid = false;
    // virtual ~MainWindow();
private:
    class ListColumns : public Gtk::TreeModelColumnRecord {
    public:
        ListColumns()
        {
            Gtk::TreeModelColumnRecord::add(net_primary);
            Gtk::TreeModelColumnRecord::add(net_secondary);
            Gtk::TreeModelColumnRecord::add(uuid);
        }
        Gtk::TreeModelColumn<Glib::ustring> net_primary;
        Gtk::TreeModelColumn<Glib::ustring> net_secondary;
        Gtk::TreeModelColumn<UUID> uuid;
    };
    ListColumns list_columns;

    Gtk::TreeView *view;
    Glib::RefPtr<Gtk::ListStore> store;

    void ok_clicked();
    void row_activated(const Gtk::TreeModel::Path &path, Gtk::TreeViewColumn *column);
};
} // namespace horizon
