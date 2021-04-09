#pragma once
#include <gtkmm.h>
#include <deque>
#include "pool-update/pool-update.hpp"
#include <set>
#include "util/uuid.hpp"
namespace horizon {
class PoolUpdateErrorDialog : public Gtk::Dialog {
public:
    PoolUpdateErrorDialog(Gtk::Window *parent,
                          const std::list<std::tuple<PoolUpdateStatus, std::string, std::string>> &errors);

private:
    class ListColumns : public Gtk::TreeModelColumnRecord {
    public:
        ListColumns()
        {
            Gtk::TreeModelColumnRecord::add(filename);
            Gtk::TreeModelColumnRecord::add(error);
        }
        Gtk::TreeModelColumn<Glib::ustring> filename;
        Gtk::TreeModelColumn<Glib::ustring> error;
    };
    ListColumns list_columns;

    Gtk::TreeView *view;
    Glib::RefPtr<Gtk::ListStore> store;
};
} // namespace horizon
