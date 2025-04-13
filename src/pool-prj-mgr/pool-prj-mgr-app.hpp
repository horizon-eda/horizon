#pragma once
#include <gtkmm.h>
#include "util/uuid.hpp"
#include "nlohmann/json_fwd.hpp"
#include <map>
#include <set>
#include <zmq.hpp>
#include <glibmm/datetime.h>
#include "util/win32_undef.hpp"
#include "preferences/preferences.hpp"
#include "logger/log_dispatcher.hpp"

namespace horizon {
using json = nlohmann::json;

class PoolProjectManagerApplication : public Gtk::Application {
protected:
    PoolProjectManagerApplication();

public:
    static Glib::RefPtr<PoolProjectManagerApplication> create();
    const std::string &get_ep_broadcast() const;
    void send_json(int pid, const json &j);
    zmq::context_t zctx;


    void add_recent_item(const std::string &path);
    class UserConfig {
    private:
        friend PoolProjectManagerApplication;
        UserConfig() = default;
        void load(const std::string &filename);
        void save(const std::string &filename);

    public:
        std::map<std::string, Glib::DateTime> recent_items;
        struct RecentItemTitleCacheItem {
            int64_t mtime;
            std::string title;
        };
        std::map<std::string, RecentItemTitleCacheItem> recent_items_title_cache;
        std::deque<UUID> part_favorites;
        bool pool_doc_info_bar_dismissed = false;
        std::string project_author;
        std::string project_base_path;
        UUID project_pool;
    };

    UserConfig user_config;

    void close_appwindows(std::set<Gtk::Window *> wins);
    Preferences &get_preferences();

    class PoolProjectManagerAppWindow &open_pool_or_project(const std::string &pool_json, guint32 timestamp = 0,
                                                            const std::string &token = "");

    class PreferencesWindow *show_preferences_window(guint32 timestamp = 0, const std::string &token = "");
    class PoolsWindow *show_pools_window(guint32 timestamp = 0, const std::string &token = "");
    class LogWindow *show_log_window(guint32 timestamp = 0, const std::string &token = "");

    typedef sigc::signal<void, std::vector<std::string>> type_signal_pool_items_edited;
    type_signal_pool_items_edited signal_pool_items_edited()
    {
        return s_signal_pool_items_edited;
    }

    typedef sigc::signal<void, std::string, std::vector<std::string>> type_signal_pool_updated;
    type_signal_pool_updated signal_pool_updated()
    {
        return s_signal_pool_updated;
    }

    typedef sigc::signal<void> type_signal_recent_items_changed;
    type_signal_recent_items_changed signal_recent_items_changed()
    {
        return s_signal_recent_items_changed;
    }

    struct CloseOrHomeWindow {
        class PoolProjectManagerAppWindow &win;
        bool close = true;
    };
    bool close_windows(std::vector<CloseOrHomeWindow> windows);

    bool present_existing_window(const std::string &path);

protected:
    // Override default signal handlers:
    void on_activate() override;
    void on_startup() override;
    void on_shutdown();
    void on_open(const Gio::Application::type_vec_files &files, const Glib::ustring &hint) override;

    std::string sock_broadcast_ep;


private:
    class PoolProjectManagerAppWindow *create_appwindow();
    void on_hide_window(Gtk::Window *window);
    void on_action_quit();
    void on_action_new_window();
    void on_action_about();
    Preferences preferences;
    class PreferencesWindow *preferences_window = nullptr;
    class PoolsWindow *pools_window = nullptr;

    LogDispatcher log_dispatcher;
    class LogWindow *log_window = nullptr;

    type_signal_pool_items_edited s_signal_pool_items_edited;
    type_signal_pool_updated s_signal_pool_updated;
    type_signal_recent_items_changed s_signal_recent_items_changed;

    std::string get_config_filename();

public:
    const UUID ipc_cookie;
    zmq::socket_t sock_broadcast;
};
} // namespace horizon
