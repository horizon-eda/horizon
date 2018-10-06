#pragma once
#include <gtkmm.h>
#include "util/uuid.hpp"

namespace horizon {
class ComponentSelector : public Gtk::Box {
public:
    ComponentSelector(class Block *b);
    UUID get_selected_component();
    void select_component(const UUID &uu);

    typedef sigc::signal<void, UUID> type_signal_selected;
    type_signal_selected signal_activated()
    {
        return s_signal_activated;
    }
    void update();

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
    Block *block;

    Gtk::TreeView *view;
    Glib::RefPtr<Gtk::ListStore> store;

    // type_signal_selected s_signal_selected;
    type_signal_selected s_signal_activated;
    void row_activated(const Gtk::TreeModel::Path &path, Gtk::TreeViewColumn *column);
};
} // namespace horizon
