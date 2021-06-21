#pragma once
#include <gtkmm.h>
#include "util/uuid.hpp"
#include "util/changeable.hpp"
#include "common/common.hpp"
#include <set>
#include "nlohmann/json.hpp"
#include "grid_controller.hpp"
#include "common/grid_settings.hpp"

namespace horizon {
using json = nlohmann::json;

class GridsWindow : public Gtk::Window, public Changeable {
public:
    static GridsWindow *create(Gtk::Window *p, GridController &b, GridSettings &settings);
    GridsWindow(BaseObjectType *cobject, const Glib::RefPtr<Gtk::Builder> &x, GridController &b,
                GridSettings &settings);
    void set_select_mode(bool select_mode);
    bool has_grids() const;

private:
    GridController &grid_controller;
    GridSettings &settings;

    bool select_mode = false;

    void row_from_grid(Gtk::TreeModel::Row &row, const GridSettings::Grid &g) const;

    class ListColumns : public Gtk::TreeModelColumnRecord {
    public:
        ListColumns()
        {
            Gtk::TreeModelColumnRecord::add(uuid);
            Gtk::TreeModelColumnRecord::add(name);
            Gtk::TreeModelColumnRecord::add(spacing);
            Gtk::TreeModelColumnRecord::add(origin);
        }
        Gtk::TreeModelColumn<UUID> uuid;
        Gtk::TreeModelColumn<Glib::ustring> name;
        Gtk::TreeModelColumn<Glib::ustring> spacing;
        Gtk::TreeModelColumn<Glib::ustring> origin;
    };
    ListColumns list_columns;

    Glib::RefPtr<Gtk::ListStore> store;
    Gtk::TreeView *treeview = nullptr;

    void update_buttons();

    Gtk::Box *button_box = nullptr;
    Gtk::Button *button_ok = nullptr;
    Gtk::Button *button_cancel = nullptr;
    Gtk::Button *button_apply = nullptr;
    Gtk::Button *button_remove = nullptr;
    Gtk::HeaderBar *headerbar = nullptr;
};

} // namespace horizon
