#pragma once
#include <array>
#include <gtkmm.h>
#include <set>
#include "util/window_state_store.hpp"
#include "block/bom.hpp"
#include "util/changeable.hpp"

namespace horizon {

class BOMExportWindow : public Gtk::Window, public Changeable {

public:
    BOMExportWindow(BaseObjectType *cobject, const Glib::RefPtr<Gtk::Builder> &x, class Block *block,
                    class BOMExportSettings *settings);
    static BOMExportWindow *create(Gtk::Window *p, class Block *block, class BOMExportSettings *settings);

    void set_can_export(bool v);
    void generate();
    void update_preview();

private:
    class Block *block;
    class BOMExportSettings *settings;

    Gtk::Button *export_button = nullptr;
    Gtk::ComboBoxText *sort_column_combo = nullptr;
    Gtk::ComboBoxText *sort_order_combo = nullptr;
    Gtk::Revealer *done_revealer = nullptr;
    Gtk::Label *done_label = nullptr;
    Gtk::Entry *filename_entry = nullptr;
    Gtk::Button *filename_button = nullptr;

    class ListColumns : public Gtk::TreeModelColumnRecord {
    public:
        ListColumns()
        {
            Gtk::TreeModelColumnRecord::add(name);
            Gtk::TreeModelColumnRecord::add(column);
        }
        Gtk::TreeModelColumn<BOMColumn> column;
        Gtk::TreeModelColumn<Glib::ustring> name;
    };
    ListColumns list_columns;

    Glib::RefPtr<Gtk::ListStore> columns_store;
    Glib::RefPtr<Gtk::TreeModelFilter> columns_available;

    Glib::RefPtr<Gtk::ListStore> columns_store_included;

    Gtk::TreeView *cols_available_tv = nullptr;
    Gtk::TreeView *cols_included_tv = nullptr;
    Gtk::TreeView *preview_tv = nullptr;

    Gtk::Button *col_inc_button = nullptr;
    Gtk::Button *col_excl_button = nullptr;
    Gtk::Button *col_up_button = nullptr;
    Gtk::Button *col_down_button = nullptr;

    WindowStateStore state_store;

    void incl_excl_col(bool incl);
    void up_down_col(bool up);
    void update_incl_excl_sensitivity();
    void update_cols_included();

    void flash(const std::string &s);
    sigc::connection flash_connection;

    class ListColumnsPreview : public Gtk::TreeModelColumnRecord {
    public:
        ListColumnsPreview()
        {
            Gtk::TreeModelColumnRecord::add(row);
        }
        Gtk::TreeModelColumn<BOMRow> row;
    };
    ListColumnsPreview list_columns_preview;

    Glib::RefPtr<Gtk::ListStore> bom_store;
};
} // namespace horizon
