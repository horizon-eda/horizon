#pragma once
#include <gtkmm.h>
#include <array>
#include <set>
#include "util/uuid.hpp"
namespace horizon {


class SelectIncludedBoardDialog : public Gtk::Dialog {
public:
    SelectIncludedBoardDialog(Gtk::Window *parent, const class Board &brd);
    UUID selected_uuid;
    bool valid = false;
    // virtual ~MainWindow();
private:
    class ListColumns : public Gtk::TreeModelColumnRecord {
    public:
        ListColumns()
        {
            Gtk::TreeModelColumnRecord::add(name);
            Gtk::TreeModelColumnRecord::add(uuid);
        }
        Gtk::TreeModelColumn<Glib::ustring> name;
        Gtk::TreeModelColumn<UUID> uuid;
    };
    ListColumns list_columns;

    Gtk::TreeView *view;
    Glib::RefPtr<Gtk::ListStore> store;

    void ok_clicked();
    void row_activated(const Gtk::TreeModel::Path &path, Gtk::TreeViewColumn *column);
};
} // namespace horizon
