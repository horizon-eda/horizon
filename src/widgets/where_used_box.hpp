#pragma once
#include <gtkmm.h>
#include "common/common.hpp"
#include "util/uuid.hpp"
#include "util/pool_goto_provider.hpp"

namespace horizon {
class WhereUsedBox : public Gtk::Box, public PoolGotoProvider {
public:
    WhereUsedBox(class Pool &pool);

    void load(ObjectType type, const UUID &uu);
    void clear();

private:
    Pool &pool;

    class ListColumns : public Gtk::TreeModelColumnRecord {
    public:
        ListColumns()
        {
            Gtk::TreeModelColumnRecord::add(name);
            Gtk::TreeModelColumnRecord::add(type);
            Gtk::TreeModelColumnRecord::add(uuid);
        }
        Gtk::TreeModelColumn<Glib::ustring> name;
        Gtk::TreeModelColumn<ObjectType> type;
        Gtk::TreeModelColumn<UUID> uuid;
    };
    ListColumns list_columns;

    Gtk::TreeView *view;
    Glib::RefPtr<Gtk::ListStore> store;
    void row_activated(const Gtk::TreeModel::Path &path, Gtk::TreeViewColumn *column);
};
} // namespace horizon
