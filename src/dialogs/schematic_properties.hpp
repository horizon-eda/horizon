#pragma once
#include <gtkmm.h>
#include <array>
#include <set>
#include "util/uuid.hpp"
namespace horizon {


class SchematicPropertiesDialog : public Gtk::Dialog {
public:
    SchematicPropertiesDialog(Gtk::Window *parent, class IDocumentSchematic &c);


private:
    IDocumentSchematic &doc;
    class Schematic &sch;

    class TreeColumns : public Gtk::TreeModelColumnRecord {
    public:
        TreeColumns()
        {
            Gtk::TreeModelColumnRecord::add(name);
            Gtk::TreeModelColumnRecord::add(sheet);
            Gtk::TreeModelColumnRecord::add(block);
        }
        Gtk::TreeModelColumn<Glib::ustring> name;
        Gtk::TreeModelColumn<UUID> sheet;
        Gtk::TreeModelColumn<UUID> block;
    };
    TreeColumns tree_columns;

    Gtk::TreeView *view = nullptr;
    Glib::RefPtr<Gtk::TreeStore> store;

    void sheets_to_row(Gtk::TreeModel::Row &row, const class Schematic &sch, const UUID &block_uuid);
    void selection_changed();

    Gtk::Box *box = nullptr;

    Gtk::Widget *current = nullptr;

    void ok_clicked();
    void add_block();
    void update_view();
};
} // namespace horizon
