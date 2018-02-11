#include "prj-mgr-app.hpp"
#include "prj-mgr-app_win.hpp"
#include "prj-mgr-prefs.hpp"
#include "util/recent_util.hpp"
#include "util/util.hpp"
#include <fstream>
#include <glibmm/miscutils.h>
#include "nlohmann/json.hpp"

namespace horizon {

ProjectManagerPool::ProjectManagerPool(const json &j, const std::string &p)
    : path(p), name(j.at("name").get<std::string>()), uuid(j.at("uuid").get<std::string>())
{
    if (j.count("default_via")) {
        default_via = UUID(j.at("default_via").get<std::string>());
    }
}

ProjectManagerApplication::ProjectManagerApplication()
    : Gtk::Application("net.carrotIndustries.horizon.prj-mgr", Gio::APPLICATION_HANDLES_OPEN),
      sock_broadcast(zctx, ZMQ_PUB)
{
    sock_broadcast.bind("tcp://127.0.0.1:*");
    char ep[1024];
    size_t sz = sizeof(ep);
    sock_broadcast.getsockopt(ZMQ_LAST_ENDPOINT, ep, &sz);
    sock_broadcast_ep = ep;
}

const std::string &ProjectManagerApplication::get_ep_broadcast() const
{
    return sock_broadcast_ep;
}

void ProjectManagerApplication::send_json(int pid, const json &j)
{
    std::string s = j.dump();
    zmq::message_t msg(s.size() + 5);
    memcpy(msg.data(), &pid, 4);
    memcpy(((uint8_t *)msg.data()) + 4, s.c_str(), s.size());
    auto m = (char *)msg.data();
    m[msg.size() - 1] = 0;
    sock_broadcast.send(msg);
}

Glib::RefPtr<ProjectManagerApplication> ProjectManagerApplication::create()
{
    return Glib::RefPtr<ProjectManagerApplication>(new ProjectManagerApplication());
}

ProjectManagerAppWindow *ProjectManagerApplication::create_appwindow()
{
    auto appwindow = ProjectManagerAppWindow::create(this);

    // Make sure that the application runs for as long this window is still
    // open.
    add_window(*appwindow);

    // Gtk::Application::add_window() connects a signal handler to the window's
    // signal_hide(). That handler removes the window from the application.
    // If it's the last window to be removed, the application stops running.
    // Gtk::Window::set_application() does not connect a signal handler, but is
    // otherwise equivalent to Gtk::Application::add_window().

    // Delete the window when it is hidden.
    appwindow->signal_hide().connect(
            sigc::bind(sigc::mem_fun(*this, &ProjectManagerApplication::on_hide_window), appwindow));

    return appwindow;
}

void ProjectManagerApplication::on_activate()
{
    // The application has been started, so let's show a window.
    auto appwindow = create_appwindow();
    appwindow->present();
}

void ProjectManagerApplication::add_pool(const std::string &p)
{
    std::ifstream ifs(p);
    if (ifs.is_open()) {
        json k;
        ifs >> k;
        UUID uu(k.at("uuid").get<std::string>());
        pools.emplace(uu, ProjectManagerPool(k, Glib::path_get_dirname(p)));
    }
    ifs.close();
}

std::string ProjectManagerApplication::get_config_filename()
{
    return Glib::build_filename(get_config_dir(), "project-manager.json");
}

void ProjectManagerApplication::on_startup()
{
    // Call the base class's implementation.
    Gtk::Application::on_startup();

    // Add actions and keyboard accelerators for the application menu.
    add_action("preferences", sigc::mem_fun(*this, &ProjectManagerApplication::on_action_preferences));
    add_action("quit", sigc::mem_fun(*this, &ProjectManagerApplication::on_action_quit));
    set_accel_for_action("app.quit", "<Ctrl>Q");

    auto refBuilder = Gtk::Builder::create();
    refBuilder->add_from_resource("/net/carrotIndustries/horizon/prj-mgr/app_menu.ui");

    auto object = refBuilder->get_object("appmenu");
    auto app_menu = Glib::RefPtr<Gio::MenuModel>::cast_dynamic(object);
    if (app_menu)
        set_app_menu(app_menu);

    auto config_filename = get_config_filename();
    if (Glib::file_test(config_filename, Glib::FILE_TEST_EXISTS)) {
        json j;
        {
            std::ifstream ifs(config_filename);
            if (!ifs.is_open()) {
                throw std::runtime_error("file " + config_filename + " not opened");
            }
            ifs >> j;
            ifs.close();
        }

        if (j.count("pools")) {
            const json &o = j.at("pools");
            for (auto it = o.cbegin(); it != o.cend(); ++it) {
                auto pool_filename = it.value().get<std::string>();
                add_pool(pool_filename);
            }
        }
        if (j.count("part_favorites")) {
            const json &o = j.at("part_favorites");
            for (auto it = o.cbegin(); it != o.cend(); ++it) {
                part_favorites.push_back(it.value().get<std::string>());
            }
        }
        recent_from_json(recent_items, j);
    }

    Gtk::IconTheme::get_default()->add_resource_path("/net/carrotIndustries/horizon/icons");

    Gtk::Window::set_default_icon_name("horizon-eda");

    signal_shutdown().connect(sigc::mem_fun(this, &ProjectManagerApplication::on_shutdown));
}

void ProjectManagerApplication::on_shutdown()
{
    auto config_filename = get_config_filename();

    json j;
    {
        json k;
        for (const auto &it : pools) {
            k.push_back(Glib::build_filename(it.second.path, "pool.json"));
        }
        j["pools"] = k;
    }
    {
        json k;
        for (const auto &it : part_favorites) {
            k.push_back((std::string)it);
        }
        j["part_favorites"] = k;
    }

    for (const auto &it : recent_items) {
        j["recent"][it.first] = it.second.to_unix();
    }

    save_json_to_file(config_filename, j);
}

void ProjectManagerApplication::on_open(const Gio::Application::type_vec_files &files, const Glib::ustring & /* hint */)
{

    // The application has been asked to open some files,
    // so let's open a new view for each one.
    ProjectManagerAppWindow *appwindow = nullptr;
    auto windows = get_windows();
    if (windows.size() > 0)
        appwindow = dynamic_cast<ProjectManagerAppWindow *>(windows[0]);

    if (!appwindow)
        appwindow = create_appwindow();

    for (const auto &file : files)
        appwindow->open_file_view(file);

    appwindow->present();
}

void ProjectManagerApplication::on_hide_window(Gtk::Window *window)
{
    delete window;
}

void ProjectManagerApplication::on_action_preferences()
{
    auto prefs_dialog = new ProjectManagerPrefs(dynamic_cast<Gtk::ApplicationWindow *>(get_active_window()));
    prefs_dialog->present();
    prefs_dialog->signal_hide().connect([prefs_dialog] { delete prefs_dialog; });
}

void ProjectManagerApplication::on_action_quit()
{
    // Gio::Application::quit() will make Gio::Application::run() return,
    // but it's a crude way of ending the program. The window is not removed
    // from the application. Neither the window's nor the application's
    // destructors will be called, because there will be remaining reference
    // counts in both of them. If we want the destructors to be called, we
    // must remove the window from the application. One way of doing this
    // is to hide the window. See comment in create_appwindow().

    bool close_failed = false;
    auto windows = get_windows();
    for (auto window : windows) {
        auto win = dynamic_cast<ProjectManagerAppWindow *>(window);
        if (!win->close_project()) {
            close_failed = true;
        }
        else {
            win->hide();
        }
    }

    // Not really necessary, when Gtk::Widget::hide() is called, unless
    // Gio::Application::hold() has been called without a corresponding call
    // to Gio::Application::release().
    if (!close_failed)
        quit();
}
} // namespace horizon
