#pragma once
#include <gtkmm.h>
#include "util/uuid.hpp"
#include "nlohmann/json_fwd.hpp"
#include <map>
#include <zmq.hpp>
#include <glibmm/datetime.h>

namespace horizon {
using json = nlohmann::json;

class PoolProjectManagerPool {
public:
    PoolProjectManagerPool(const json &j, const std::string &p);
    std::string path;
    std::string name;
    UUID uuid;
    UUID default_via;
};

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

    std::map<UUID, PoolProjectManagerPool> pools;
    void add_pool(const std::string &p);
    std::deque<UUID> part_favorites;

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
    void on_action_preferences();
    void load_from_config(const std::string &config_filename);


public:
    zmq::socket_t sock_broadcast;
};
} // namespace horizon
