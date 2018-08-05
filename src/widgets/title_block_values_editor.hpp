#pragma once
#include <gtkmm.h>
#include "util/uuid.hpp"

namespace horizon {
class TitleBlockValuesEditor : public Gtk::Box {
public:
    TitleBlockValuesEditor(std::map<std::string, std::string> &v);

private:
    class ListColumns : public Gtk::TreeModelColumnRecord {
    public:
        ListColumns()
        {
            Gtk::TreeModelColumnRecord::add(key);
            Gtk::TreeModelColumnRecord::add(value);
        }
        Gtk::TreeModelColumn<Glib::ustring> key;
        Gtk::TreeModelColumn<Glib::ustring> value;
    };
    ListColumns list_columns;

    std::map<std::string, std::string> &values;

    Gtk::TreeView *view;
    Glib::RefPtr<Gtk::ListStore> store;
    Gtk::ToolButton *tb_remove = nullptr;
};
} // namespace horizon
