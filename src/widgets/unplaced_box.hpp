#pragma once
#include <gtkmm.h>
#include <map>
#include <set>
#include "util/uuid.hpp"
#include "util/uuid_path.hpp"

namespace horizon {
class UnplacedBox : public Gtk::Box {
public:
    UnplacedBox(const std::string &title);

    void update(const std::map<UUIDPath<2>, std::string> &items);
    typedef sigc::signal<void, std::vector<UUIDPath<2>>> type_signal_place;
    type_signal_place signal_place()
    {
        return s_signal_place;
    }

private:
    class ListColumns : public Gtk::TreeModelColumnRecord {
    public:
        ListColumns()
        {
            Gtk::TreeModelColumnRecord::add(text);
            Gtk::TreeModelColumnRecord::add(uuid);
        }
        Gtk::TreeModelColumn<Glib::ustring> text;
        Gtk::TreeModelColumn<UUIDPath<2>> uuid;
    };
    ListColumns list_columns;

    Gtk::TreeView *view = nullptr;
    Glib::RefPtr<Gtk::ListStore> store;
    Gtk::ToolButton *button_place = nullptr;

    type_signal_place s_signal_place;
    void row_activated(const Gtk::TreeModel::Path &path, Gtk::TreeViewColumn *column);
};
} // namespace horizon
