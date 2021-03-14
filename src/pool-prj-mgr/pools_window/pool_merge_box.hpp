#pragma once
#include <gtkmm.h>
#include "common/common.hpp"
#include "util/uuid.hpp"
#include "nlohmann/json.hpp"
#include "pool_status_provider.hpp"

namespace horizon {
using json = nlohmann::json;

class PoolMergeBox2 : public Gtk::Box {
public:
    PoolMergeBox2(BaseObjectType *cobject, const Glib::RefPtr<Gtk::Builder> &x, PoolStatusProviderPoolManager &prv);
    static PoolMergeBox2 *create(PoolStatusProviderPoolManager &prv);


private:
    Gtk::TreeView *pool_item_view = nullptr;
    Gtk::Stack *stack = nullptr;
    Gtk::TextView *delta_text_view = nullptr;
    Gtk::CheckButton *cb_update_layer_help = nullptr;
    Gtk::CheckButton *cb_update_tables = nullptr;
    Gtk::Menu context_menu;
    Gtk::Button *button_update = nullptr;

    PoolStatusProviderPoolManager &prv;
    void update_from_prv();
    void selection_changed();

    enum class MenuOP { CHECK, UNCHECK, TOGGLE };
    void append_context_menu_item(const std::string &name, MenuOP op);

    void action_toggled(const Glib::ustring &path);

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
        Gtk::TreeModelColumn<PoolStatusPoolManager::ItemInfo::ItemState> state;
    };
    TreeColumns list_columns;

    Glib::RefPtr<Gtk::ListStore> item_store;
};
} // namespace horizon
