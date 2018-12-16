#pragma once
#include <gtkmm.h>
#include <set>
#include "common/common.hpp"
#include "util/uuid.hpp"
#include "pool/unit.hpp"
#include "util/pool_goto_provider.hpp"

namespace horizon {

class UnitInfoBox : public Gtk::Box, public PoolGotoProvider {
public:
    UnitInfoBox(BaseObjectType *cobject, const Glib::RefPtr<Gtk::Builder> &x, class Pool &p);
    static UnitInfoBox *create(Pool &p);
    void load(const Unit *u);

private:
    Pool &pool;
    class WhereUsedBox *where_used_box = nullptr;

    Gtk::Label *label_name = nullptr;
    Gtk::Label *label_manufacturer = nullptr;

    class ListColumns : public Gtk::TreeModelColumnRecord {
    public:
        ListColumns()
        {
            Gtk::TreeModelColumnRecord::add(direction);
            Gtk::TreeModelColumnRecord::add(primary_name);
            Gtk::TreeModelColumnRecord::add(swap_group);
            Gtk::TreeModelColumnRecord::add(alt_names);
        }
        Gtk::TreeModelColumn<Pin::Direction> direction;
        Gtk::TreeModelColumn<Glib::ustring> primary_name;
        Gtk::TreeModelColumn<unsigned int> swap_group;
        Gtk::TreeModelColumn<Glib::ustring> alt_names;
    };
    ListColumns list_columns;

    Gtk::TreeView *view = nullptr;
    Glib::RefPtr<Gtk::ListStore> store;
};
} // namespace horizon
