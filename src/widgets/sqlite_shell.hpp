#pragma once
#include <gtkmm.h>
#include "util/sqlite.hpp"

namespace horizon {
class SQLiteShellWindow : public Gtk::Window {
public:
    SQLiteShellWindow(const std::string &db_path);

private:
    class ListColumns : public Gtk::TreeModelColumnRecord {
    public:
        ListColumns()
        {
            Gtk::TreeModelColumnRecord::add(columns);
        }
        Gtk::TreeModelColumn<std::vector<std::string>> columns;
    };
    ListColumns list_columns;

    Glib::RefPtr<Gtk::ListStore> store;
    Gtk::Entry *entry = nullptr;
    Gtk::TreeView *tree_view = nullptr;
    Gtk::Label *status_label = nullptr;

    void run();

    SQLite::Database db;
};
} // namespace horizon
