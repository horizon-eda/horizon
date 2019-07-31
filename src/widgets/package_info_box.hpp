#pragma once
#include <gtkmm.h>
#include <set>
#include "common/common.hpp"
#include "util/uuid.hpp"
#include "util/pool_goto_provider.hpp"

namespace horizon {

class PackageInfoBox : public Gtk::Box, public PoolGotoProvider {
public:
    PackageInfoBox(BaseObjectType *cobject, const Glib::RefPtr<Gtk::Builder> &x, class Pool &p);
    static PackageInfoBox *create(Pool &p);
    void load(const class Package *p);

private:
    Pool &pool;
    class WhereUsedBox *where_used_box = nullptr;

    Gtk::Label *label_name = nullptr;
    Gtk::Label *label_manufacturer = nullptr;
    Gtk::Label *label_alt_for = nullptr;
    Gtk::Label *label_tags = nullptr;

    class ListColumns : public Gtk::TreeModelColumnRecord {
    public:
        ListColumns()
        {
            Gtk::TreeModelColumnRecord::add(padstack);
            Gtk::TreeModelColumnRecord::add(count);
            Gtk::TreeModelColumnRecord::add(specific);
        }
        Gtk::TreeModelColumn<const class Padstack *> padstack;
        Gtk::TreeModelColumn<unsigned int> count;
        Gtk::TreeModelColumn<bool> specific;
    };
    ListColumns list_columns;

    Gtk::TreeView *view = nullptr;
    Glib::RefPtr<Gtk::ListStore> store;
};
} // namespace horizon
