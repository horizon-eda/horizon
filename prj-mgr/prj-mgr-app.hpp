#pragma once
#include <gtkmm.h>
#include "uuid.hpp"
#include "json_fwd.hpp"
#include <map>
#ifdef G_OS_WIN32
	#include "zmq_win.hpp"
#else
	#include <zmq.hpp>
#endif

namespace horizon {
	using json = nlohmann::json;

	class ProjectManagerPool {
		public:
			ProjectManagerPool(const json &j, const std::string &p);
			std::string path;
			std::string name;
			UUID uuid;
	};


	class ProjectManagerApplication: public Gtk::Application {
		protected:
			ProjectManagerApplication();

		public:
			static Glib::RefPtr<ProjectManagerApplication> create();
			std::map<UUID, ProjectManagerPool> pools;
			void add_pool(const std::string &p);
			std::string get_config_filename();
			const std::string &get_ep_broadcast() const;
			void send_json(int pid, const json &j);

		protected:
			// Override default signal handlers:
			void on_activate() override;
			void on_startup() override;
			void on_shutdown();
			void on_open(const Gio::Application::type_vec_files& files,
				const Glib::ustring& hint) override;
			zmq::context_t zctx;

			std::string sock_broadcast_ep;


		private:
			class ProjectManagerAppWindow* create_appwindow();
			void on_hide_window(Gtk::Window* window);
			void on_action_quit();
			void on_action_preferences();

		public:
			zmq::socket_t sock_broadcast;



	};

}
