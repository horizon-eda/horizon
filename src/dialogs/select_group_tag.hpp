#pragma once
#include <gtkmm.h>
#include <array>
#include <set>
#include "common/common.hpp"
#include "util/uuid.hpp"
#include "pool/unit.hpp"

namespace horizon {


class SelectGroupTagDialog : public Gtk::Dialog {
public:
    SelectGroupTagDialog(Gtk::Window *parent, const class Block *block, bool tag_mode);
    UUID selected_uuid;
    bool selection_valid = false;

private:
    class ListColumns : public Gtk::TreeModelColumnRecord {
    public:
        ListColumns()
        {
            Gtk::TreeModelColumnRecord::add(name);
            Gtk::TreeModelColumnRecord::add(uuid);
            Gtk::TreeModelColumnRecord::add(components);
        }
        Gtk::TreeModelColumn<Glib::ustring> name;
        Gtk::TreeModelColumn<Glib::ustring> components;
        Gtk::TreeModelColumn<UUID> uuid;
    };
    ListColumns list_columns;

    Gtk::TreeView *view;
    Glib::RefPtr<Gtk::ListStore> store;

    void ok_clicked();
    void row_activated(const Gtk::TreeModel::Path &path, Gtk::TreeViewColumn *column);
};
} // namespace horizon
