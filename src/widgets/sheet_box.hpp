#pragma once
#include <gtkmm.h>
#include "util/uuid.hpp"
#include "util/uuid_vec.hpp"
#include <optional>

namespace horizon {
class SheetBox : public Gtk::Box {
public:
    SheetBox(class CoreSchematic &c);

    void update();
    void clear_highlights();
    void add_highlights(const UUID &sheet, const UUIDVec &path);
    void select_sheet(const UUID &sheet_uuid);
    void go_to_instance(const UUIDVec &path, const UUID &sheet = UUID());
    void go_to_block_symbol(const UUID &uu);
    typedef sigc::signal<void, UUID, UUID, UUIDVec> type_signal_select_sheet;
    type_signal_select_sheet signal_select_sheet()
    {
        return s_signal_select_sheet;
    }

    typedef sigc::signal<void> type_signal_edit_more;
    type_signal_edit_more signal_edit_more()
    {
        return s_signal_edit_more;
    }

    typedef sigc::signal<void, UUID> type_signal_place_block;
    type_signal_place_block signal_place_block()
    {
        return s_signal_place_block;
    }

private:
    enum class RowType { SHEET, BLOCK, BLOCK_INSTANCE, SEPARATOR };

    class TreeColumns : public Gtk::TreeModelColumnRecord {
    public:
        TreeColumns()
        {
            Gtk::TreeModelColumnRecord::add(name);
            Gtk::TreeModelColumnRecord::add(sheet);
            Gtk::TreeModelColumnRecord::add(block);
            Gtk::TreeModelColumnRecord::add(index);
            Gtk::TreeModelColumnRecord::add(has_warnings);
            Gtk::TreeModelColumnRecord::add(has_highlights);
            Gtk::TreeModelColumnRecord::add(type);
            Gtk::TreeModelColumnRecord::add(instance_path);
        }
        Gtk::TreeModelColumn<Glib::ustring> name;
        Gtk::TreeModelColumn<UUID> sheet;
        Gtk::TreeModelColumn<UUID> block;
        Gtk::TreeModelColumn<unsigned int> index;
        Gtk::TreeModelColumn<bool> has_warnings;
        Gtk::TreeModelColumn<bool> has_highlights;
        Gtk::TreeModelColumn<RowType> type;
        Gtk::TreeModelColumn<UUIDVec> instance_path;
    };
    TreeColumns tree_columns;

    CoreSchematic &core;
    Gtk::Button *add_button = nullptr;
    Gtk::Button *remove_button = nullptr;
    Gtk::Button *move_up_button = nullptr;
    Gtk::Button *move_down_button = nullptr;

    Gtk::TreeView *view;
    Glib::RefPtr<Gtk::TreeStore> store;

    Gtk::Menu menu;

    type_signal_select_sheet s_signal_select_sheet;
    type_signal_edit_more s_signal_edit_more;
    void selection_changed(void);
    void remove_clicked(void);
    void name_edited(const Glib::ustring &path, const Glib::ustring &new_text);
    void sheet_move(int dir);

    bool updating = false;

    void sheets_to_row(std::function<Gtk::TreeModel::Row()> make_row, const class Schematic &sch,
                       const UUID &block_uuid, const UUIDVec &instance_path, bool in_hierarchy);
    std::optional<Gtk::TreeModel::iterator> last_iter;

    type_signal_place_block s_signal_place_block;
    class CellRendererButton *cr_place_block = nullptr;
};
} // namespace horizon
