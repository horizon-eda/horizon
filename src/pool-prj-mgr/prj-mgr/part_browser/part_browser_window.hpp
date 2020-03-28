#pragma once
#include "common/common.hpp"
#include "pool/part.hpp"
#include "pool/pool.hpp"
#include "pool/pool_parametric.hpp"
#include "util/uuid.hpp"
#include "util/window_state_store.hpp"
#include <array>
#include <gtkmm.h>
#include <set>
#include "util/item_set.hpp"

namespace horizon {

class PartBrowserWindow : public Gtk::Window {
public:
    PartBrowserWindow(BaseObjectType *cobject, const Glib::RefPtr<Gtk::Builder> &x, const std::string &pool_path,
                      std::deque<UUID> &favs);
    static PartBrowserWindow *create(Gtk::Window *p, const std::string &pool_path, std::deque<UUID> &favs);
    typedef sigc::signal<void, UUID> type_signal_place_part;
    type_signal_place_part signal_place_part()
    {
        return s_signal_place_part;
    }
    type_signal_place_part signal_assign_part()
    {
        return s_signal_assign_part;
    }

    typedef sigc::signal<void, ItemSet> type_signal_open_pool_cache_window;
    type_signal_open_pool_cache_window signal_open_pool_cache_window()
    {
        return s_signal_open_pool_cache_window;
    }

    void placed_part(const UUID &uu);
    void go_to_part(const UUID &uu);
    void set_can_assign(bool v);
    void set_pool_cache_status(const class PoolCacheStatus &st);

private:
    Gtk::Menu *add_search_menu = nullptr;
    Gtk::Notebook *notebook = nullptr;
    Gtk::Button *place_part_button = nullptr;
    Gtk::Button *assign_part_button = nullptr;
    Gtk::ToggleButton *fav_button = nullptr;
    Gtk::ListBox *lb_favorites = nullptr;
    Gtk::ListBox *lb_recent = nullptr;
    Gtk::InfoBar *out_of_date_info_bar = nullptr;
    class PartPreview *preview = nullptr;
    class PoolBrowserPart *add_search(const UUID &part = UUID());
    class PoolBrowserParametric *add_search_parametric(const std::string &table_name);
    void handle_switch_page(Gtk::Widget *w, guint index);
    void handle_fav_toggled();
    void update_favorites();
    void update_recents();
    void handle_favorites_selected(Gtk::ListBoxRow *row);
    void handle_favorites_activated(Gtk::ListBoxRow *row);
    void handle_place_part();
    void handle_assign_part();
    sigc::connection fav_toggled_conn;
    std::set<Gtk::Widget *> search_views;
    Pool pool;
    PoolParametric pool_parametric;
    UUID part_current;
    void update_part_current();
    void update_out_of_date_info_bar();
    ItemSet items_out_of_date;
    std::deque<UUID> &favorites;
    std::deque<UUID> recents;

    type_signal_place_part s_signal_place_part;
    type_signal_place_part s_signal_assign_part;
    type_signal_open_pool_cache_window s_signal_open_pool_cache_window;
    bool can_assign = false;
    const class PoolCacheStatus *pool_cache_status = nullptr;

    WindowStateStore state_store;
};
} // namespace horizon
