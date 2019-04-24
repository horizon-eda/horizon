#pragma once
#include <gtkmm.h>
#include "util/uuid.hpp"
#include "nlohmann/json_fwd.hpp"
#include <map>
#include <set>
#include <zmq.hpp>
#include <glibmm/datetime.h>
#ifdef G_OS_WIN32
#undef ERROR
#undef DELETE
#undef DUPLICATE
#endif
#include "preferences/preferences.hpp"

namespace horizon {
using json = nlohmann::json;

class PoolProjectManagerApplication : public Gtk::Application {
protected:
    PoolProjectManagerApplication();

public:
    static Glib::RefPtr<PoolProjectManagerApplication> create();
    std::string get_config_filename();
    const std::string &get_ep_broadcast() const;
    void send_json(int pid, const json &j);
    zmq::context_t zctx;

    std::map<std::string, Glib::DateTime> recent_items;

    std::deque<UUID> part_favorites;

    void close_appwindows(std::set<Gtk::Window *> wins);
    Preferences &get_preferences();

    void open_pool(const std::string &pool_json, ObjectType type = ObjectType::INVALID, const UUID &uu = UUID(),
                   guint32 timestamp = 0);

    enum class PreferencesWindowAction { NONE, OPEN_POOL };

    void show_preferences_window(guint32 timestamp = 0, PreferencesWindowAction action = PreferencesWindowAction::NONE);

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
    void load_from_config(const std::string &config_filename);
    Preferences preferences;
    class PreferencesWindow *preferences_window = nullptr;


public:
    zmq::socket_t sock_broadcast;
};
} // namespace horizon
