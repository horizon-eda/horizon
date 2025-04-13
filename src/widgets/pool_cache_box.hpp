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
    enum class Mode { POOL_NOTEBOOK, IMP };
    PoolCacheBox(BaseObjectType *cobject, const Glib::RefPtr<Gtk::Builder> &x, class IPool &pool, Mode mode,
                 const std::set<std::string> &items_modified);
    static PoolCacheBox *create_for_pool_notebook(class IPool &pool);
    static PoolCacheBox *create_for_imp(class IPool &pool, const std::set<std::string> &items_modified);

    bool refreshed_once = false;
    void refresh_status();
    std::vector<std::string> update_checked();

    typedef sigc::signal<void, std::vector<std::string>> type_signal_pool_update;
    type_signal_pool_update signal_pool_update()
    {
        return s_signal_pool_update;
    }

    typedef sigc::signal<void> type_signal_cleanup_cache;
    type_signal_cleanup_cache signal_cleanup_cache()
    {
        return s_signal_cleanup_cache;
    }

private:
    static PoolCacheBox *create(class IPool &pool, Mode mode, const std::set<std::string> &items_modified);
    const Mode mode;

    class PoolProjectManagerApplication *app = nullptr;
    class PoolNotebook *notebook = nullptr;
    IPool &pool;

    void selection_changed();
    void update_from_pool();
    void update_item(const PoolCacheStatus::Item &it, std::vector<std::string> &filenames);
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
            Gtk::TreeModelColumnRecord::add(checked);
        }
        Gtk::TreeModelColumn<Glib::ustring> name;
        Gtk::TreeModelColumn<ObjectType> type;
        Gtk::TreeModelColumn<PoolCacheStatus::Item::State> state;
        Gtk::TreeModelColumn<PoolCacheStatus::Item> item;
        Gtk::TreeModelColumn<bool> checked;
    };
    TreeColumns tree_columns;

    Glib::RefPtr<Gtk::ListStore> item_store;

    type_signal_pool_update s_signal_pool_update;
    type_signal_cleanup_cache s_signal_cleanup_cache;
    const std::set<std::string> items_modified;
};
} // namespace horizon
