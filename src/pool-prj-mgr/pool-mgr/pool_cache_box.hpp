#pragma once
#include <gtkmm.h>
#include <set>
#include <mutex>
#include "util/uuid.hpp"
#include "common/common.hpp"
#include "nlohmann/json.hpp"
#include "pool/pool_cache_status.hpp"
#include "util/pool_goto_provider.hpp"

namespace horizon {
using json = nlohmann::json;

class PoolCacheBox : public Gtk::Box, public PoolGotoProvider {
public:
    PoolCacheBox(BaseObjectType *cobject, const Glib::RefPtr<Gtk::Builder> &x, class PoolProjectManagerApplication *app,
                 class PoolNotebook *nb, class IPool &pool);
    static PoolCacheBox *create(class PoolProjectManagerApplication *app, class PoolNotebook *nb, class IPool &pool);

    bool refreshed_once = false;
    void refresh_status();

private:
    class PoolProjectManagerApplication *app = nullptr;
    class PoolNotebook *notebook = nullptr;
    IPool &pool;

    void selection_changed();
    void update_from_pool();
    void cleanup();
    void refresh_list(const class PoolCacheStatus &status);

    Gtk::TreeView *pool_item_view = nullptr;
    Gtk::Stack *stack = nullptr;
    Gtk::TextView *delta_text_view = nullptr;
    Gtk::Button *update_from_pool_button = nullptr;
    Gtk::Label *status_label = nullptr;

    class TreeColumns : public Gtk::TreeModelColumnRecord {
    public:
        TreeColumns()
        {
            Gtk::TreeModelColumnRecord::add(name);
            Gtk::TreeModelColumnRecord::add(type);
            Gtk::TreeModelColumnRecord::add(state);
            Gtk::TreeModelColumnRecord::add(item);
        }
        Gtk::TreeModelColumn<Glib::ustring> name;
        Gtk::TreeModelColumn<ObjectType> type;
        Gtk::TreeModelColumn<PoolCacheStatus::Item::State> state;
        Gtk::TreeModelColumn<PoolCacheStatus::Item> item;
    };
    TreeColumns tree_columns;

    Glib::RefPtr<Gtk::ListStore> item_store;
};
} // namespace horizon
