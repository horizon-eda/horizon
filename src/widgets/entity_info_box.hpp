#pragma once
#include <gtkmm.h>
#include <set>
#include "common/common.hpp"
#include "util/uuid.hpp"
#include "util/pool_goto_provider.hpp"

namespace horizon {

class EntityInfoBox : public Gtk::Box, public PoolGotoProvider {
public:
    EntityInfoBox(BaseObjectType *cobject, const Glib::RefPtr<Gtk::Builder> &x, class Pool &p);
    static EntityInfoBox *create(Pool &p);
    void load(const class Entity *e);

private:
    Pool &pool;
    class WhereUsedBox *where_used_box = nullptr;

    Gtk::Label *label_name = nullptr;
    Gtk::Label *label_manufacturer = nullptr;
    Gtk::Label *label_prefix = nullptr;
    Gtk::Label *label_tags = nullptr;

    class ListColumns : public Gtk::TreeModelColumnRecord {
    public:
        ListColumns()
        {
            Gtk::TreeModelColumnRecord::add(name);
            Gtk::TreeModelColumnRecord::add(suffix);
            Gtk::TreeModelColumnRecord::add(unit);
        }
        Gtk::TreeModelColumn<Glib::ustring> name;
        Gtk::TreeModelColumn<Glib::ustring> suffix;
        Gtk::TreeModelColumn<const class Unit *> unit;
    };
    ListColumns list_columns;

    Gtk::TreeView *view = nullptr;
    Glib::RefPtr<Gtk::ListStore> store;
};
} // namespace horizon
