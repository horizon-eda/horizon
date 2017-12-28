#include "pool-mgr-app.hpp"
#include "pool-mgr-app_win.hpp"
#include <glibmm/miscutils.h>
#include <fstream>
#include "util/util.hpp"
#include <git2.h>
#include <curl/curl.h>

namespace horizon {

	PoolManagerApplication::PoolManagerApplication():
			Gtk::Application("net.carrotIndustries.horizon.pool-mgr", Gio::APPLICATION_HANDLES_OPEN), sock_broadcast(zctx, ZMQ_PUB)
	{
		sock_broadcast.bind("tcp://127.0.0.1:*");
		char ep[1024];
		size_t sz = sizeof(ep);
		sock_broadcast.getsockopt(ZMQ_LAST_ENDPOINT, ep, &sz);
		sock_broadcast_ep = ep;
	}

	const std::string &PoolManagerApplication::get_ep_broadcast() const {
		return sock_broadcast_ep;
	}

	void PoolManagerApplication::send_json(int pid, const json &j) {
		std::string s = j.dump();
		zmq::message_t msg(s.size()+5);
		memcpy(msg.data(), &pid, 4);
		memcpy(((uint8_t*)msg.data())+4, s.c_str(), s.size());
		auto m = (char*)msg.data();
		m[msg.size()-1] = 0;
		sock_broadcast.send(msg);
	}

	Glib::RefPtr<PoolManagerApplication> PoolManagerApplication::create() {
		return Glib::RefPtr<PoolManagerApplication>(new PoolManagerApplication());
	}

	PoolManagerAppWindow* PoolManagerApplication::create_appwindow() {
		auto appwindow = PoolManagerAppWindow::create(this);

		// Make sure that the application runs for as long this window is still open.
		add_window(*appwindow);

		// Gtk::Application::add_window() connects a signal handler to the window's
		// signal_hide(). That handler removes the window from the application.
		// If it's the last window to be removed, the application stops running.
		// Gtk::Window::set_application() does not connect a signal handler, but is
		// otherwise equivalent to Gtk::Application::add_window().

		// Delete the window when it is hidden.
		appwindow->signal_hide().connect(sigc::bind(sigc::mem_fun(*this,
				&PoolManagerApplication::on_hide_window), appwindow));

		return appwindow;
	}

	void PoolManagerApplication::on_activate() {
		// The application has been started, so let's show a window.
		auto appwindow = create_appwindow();
		appwindow->present();
	}

	std::string PoolManagerApplication::get_config_filename() {
		return Glib::build_filename(get_config_dir(), "pool-manager.json");
	}

	void PoolManagerApplication::on_startup() {
		// Call the base class's implementation.
		Gtk::Application::on_startup();

		git_libgit2_init();
		#ifdef G_OS_WIN32
		{
			std::string cert_file = Glib::build_filename(horizon::get_exe_dir(), "ca-bundle.crt");
			git_libgit2_opts(GIT_OPT_SET_SSL_CERT_LOCATIONS, cert_file.c_str(), NULL);
		}
		#endif
		curl_global_init(CURL_GLOBAL_ALL);

		// Add actions and keyboard accelerators for the application menu.
		//add_action("preferences", sigc::mem_fun(*this, &ProjectManagerApplication::on_action_preferences));
		add_action("quit", sigc::mem_fun(*this, &PoolManagerApplication::on_action_quit));
		set_accel_for_action("app.quit", "<Ctrl>Q");

		auto refBuilder = Gtk::Builder::create();
		refBuilder->add_from_resource("/net/carrotIndustries/horizon/pool-mgr/app_menu.ui");


		auto object = refBuilder->get_object("appmenu");
		auto app_menu = Glib::RefPtr<Gio::MenuModel>::cast_dynamic(object);
		if (app_menu)
			set_app_menu(app_menu);

		auto config_filename = get_config_filename();
		if(Glib::file_test(config_filename, Glib::FILE_TEST_IS_REGULAR)) {
			json j;
			{
				std::ifstream ifs(config_filename);
				if(!ifs.is_open()) {
					throw std::runtime_error("file "  +config_filename+ " not opened");
				}
				ifs>>j;
				ifs.close();
			}
			if(j.count("recent")) {
				const json &o = j["recent"];
				for (auto it = o.cbegin(); it != o.cend(); ++it) {
					std::string filename = it.key();
					std::cout << filename << std::endl;
					if(Glib::file_test(filename, Glib::FILE_TEST_IS_REGULAR))
						recent_items.emplace(filename, Glib::DateTime::create_now_local(it.value().get<int64_t>()));
				}
			}
		}

		Gtk::IconTheme::get_default()->add_resource_path("/net/carrotIndustries/horizon/icons");

		signal_shutdown().connect(sigc::mem_fun(this, &PoolManagerApplication::on_shutdown));
	}

	void PoolManagerApplication::on_shutdown() {
		auto config_filename = get_config_filename();

		json j;
		//j["recent"] = json::object();
		for(const auto &it: recent_items) {
			j["recent"][it.first] = it.second.to_unix();
		}
		save_json_to_file(config_filename, j);
	}

	void PoolManagerApplication::on_open(const Gio::Application::type_vec_files& files,
		const Glib::ustring& /* hint */) {

		// The application has been asked to open some files,
		// so let's open a new view for each one.
		PoolManagerAppWindow* appwindow = nullptr;
		auto windows = get_windows();
		if (windows.size() > 0)
			appwindow = dynamic_cast<PoolManagerAppWindow*>(windows[0]);

		if (!appwindow)
			appwindow = create_appwindow();

		for (const auto& file : files)
			appwindow->open_file_view(file);

		appwindow->present();
	}

	void PoolManagerApplication::on_hide_window(Gtk::Window* window) {
		delete window;
	}

	void PoolManagerApplication::on_action_quit() {
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
			auto win = dynamic_cast<PoolManagerAppWindow*>(window);
			if(!win->close_pool()) {
				close_failed = true;
			}
			else {
				win->hide();
			}
		}

		// Not really necessary, when Gtk::Widget::hide() is called, unless
		// Gio::Application::hold() has been called without a corresponding call
		// to Gio::Application::release().
		if(!close_failed)
			quit();
	}
}
