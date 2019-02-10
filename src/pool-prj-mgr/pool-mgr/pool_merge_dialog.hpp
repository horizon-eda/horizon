#pragma once
#include <gtkmm.h>
#include "common/common.hpp"
#include "util/uuid.hpp"
#include "nlohmann/json_fwd.hpp"

namespace horizon {
using json = nlohmann::json;

class PoolMergeDialog : public Gtk::Dialog {
public:
    PoolMergeDialog(Gtk::Window *parent, const std::string &local_path, const std::string &remote_path);
    bool get_merged() const;


private:
    std::string local_path;
    std::string remote_path;
    class PoolMergeBox *box = nullptr;
    void do_merge();
    void populate_store();
    void selection_changed();
    void action_toggled(const Glib::ustring &path);

    enum class ItemState {
        CURRENT,
        LOCAL_ONLY,
        REMOTE_ONLY,
        MOVED,
        CHANGED,
        MOVED_CHANGED,
    };

    class TreeColumns : public Gtk::TreeModelColumnRecord {
    public:
        TreeColumns()
        {
            Gtk::TreeModelColumnRecord::add(name);
            Gtk::TreeModelColumnRecord::add(type);
            Gtk::TreeModelColumnRecord::add(uuid);
            Gtk::TreeModelColumnRecord::add(delta);
            Gtk::TreeModelColumnRecord::add(filename_local);
            Gtk::TreeModelColumnRecord::add(filename_remote);
            Gtk::TreeModelColumnRecord::add(merge);
            Gtk::TreeModelColumnRecord::add(state);
        }
        Gtk::TreeModelColumn<Glib::ustring> name;
        Gtk::TreeModelColumn<std::string> filename_local;
        Gtk::TreeModelColumn<std::string> filename_remote;
        Gtk::TreeModelColumn<ObjectType> type;
        Gtk::TreeModelColumn<UUID> uuid;
        Gtk::TreeModelColumn<json> delta;
        Gtk::TreeModelColumn<bool> merge;
        Gtk::TreeModelColumn<ItemState> state;
    };
    TreeColumns list_columns;

    Glib::RefPtr<Gtk::ListStore> item_store;
    bool merged = false;

    std::string tables_remote, tables_local;
    std::string layer_help_remote, layer_help_local;
};
} // namespace horizon
