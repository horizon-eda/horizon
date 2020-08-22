#pragma once
#include <gtkmm.h>
#include "common/common.hpp"
#include <set>

namespace horizon {

class PoolCacheCleanupDialog : public Gtk::Dialog {
public:
    PoolCacheCleanupDialog(Gtk::Window *parent, const std::set<std::string> &filenames_delete,
                           const std::set<std::string> &models_delete, class IPool &pool);

private:
    void do_cleanup();
    void action_toggled(const Glib::ustring &path);

    class TreeColumns : public Gtk::TreeModelColumnRecord {
    public:
        TreeColumns()
        {
            Gtk::TreeModelColumnRecord::add(name);
            Gtk::TreeModelColumnRecord::add(filename);
            Gtk::TreeModelColumnRecord::add(type);
            Gtk::TreeModelColumnRecord::add(remove);
        }
        Gtk::TreeModelColumn<Glib::ustring> name;
        Gtk::TreeModelColumn<ObjectType> type;
        Gtk::TreeModelColumn<std::string> filename;
        Gtk::TreeModelColumn<bool> remove;
    };
    TreeColumns list_columns;

    Glib::RefPtr<Gtk::ListStore> item_store;
};
} // namespace horizon
