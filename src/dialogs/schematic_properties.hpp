#pragma once
#include <gtkmm.h>
#include <array>
#include <set>
#include "util/uuid.hpp"
namespace horizon {


class SchematicPropertiesDialog : public Gtk::Dialog {
public:
    SchematicPropertiesDialog(Gtk::Window *parent, class IDocumentSchematicBlockSymbol &c);


private:
    IDocumentSchematicBlockSymbol &doc;

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
    bool updating = false;

    void sheets_to_row(Gtk::TreeModel::Row &row, const class Schematic &sch, const UUID &block_uuid);
    void selection_changed();

    Gtk::Box *box = nullptr;

    Gtk::Widget *current = nullptr;
    class SheetEditor *sheet_editor = nullptr;

    Gtk::Button *remove_button = nullptr;

    void ok_clicked();
    void add_block();
    void add_sheet();
    void handle_remove();
    void update_view();
    void update_for_sheet();
    Gtk::Menu add_menu;
    Gtk::MenuItem *add_sheet_menu_item = nullptr;
};
} // namespace horizon
