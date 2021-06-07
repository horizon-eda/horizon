#pragma once
#include <gtkmm.h>
#include "util/uuid.hpp"
#include "util/changeable.hpp"
#include "common/common.hpp"
#include <set>
#include "nlohmann/json.hpp"
#include "grid_controller.hpp"

namespace horizon {
using json = nlohmann::json;

class GridsWindow : public Gtk::Window, public Changeable {
public:
    static GridsWindow *create(Gtk::Window *p, GridController &b);
    GridsWindow(BaseObjectType *cobject, const Glib::RefPtr<Gtk::Builder> &x, GridController &b);
    void set_select_mode(bool select_mode);
    bool has_grids() const;

    json serialize();
    void load_from_json(const json &j);

private:
    GridController &grid_controller;
    bool select_mode = false;

    class ListColumns : public Gtk::TreeModelColumnRecord {
    public:
        ListColumns()
        {
            Gtk::TreeModelColumnRecord::add(name);
            Gtk::TreeModelColumnRecord::add(settings);
        }
        Gtk::TreeModelColumn<Glib::ustring> name;
        Gtk::TreeModelColumn<GridSettings> settings;
    };
    ListColumns list_columns;

    Glib::RefPtr<Gtk::ListStore> store;
    Gtk::TreeView *treeview = nullptr;

    Gtk::Box *button_box = nullptr;
    Gtk::Button *button_ok = nullptr;
    Gtk::Button *button_cancel = nullptr;
    Gtk::HeaderBar *headerbar = nullptr;
};

} // namespace horizon
