#pragma once
#include <gtkmm.h>
#include "uuid.hpp"
#include "json.hpp"
#include <map>
#include <zmq.hpp>

namespace horizon {
	using json = nlohmann::json;

	class PoolManagerApplication: public Gtk::Application {
		protected:
			PoolManagerApplication();

		public:
			static Glib::RefPtr<PoolManagerApplication> create();
			std::string get_config_filename();
			const std::string &get_ep_broadcast() const;
			void send_json(int pid, const json &j);
			zmq::context_t zctx;

		protected:
			// Override default signal handlers:
			void on_activate() override;
			void on_startup() override;
			void on_shutdown();
			void on_open(const Gio::Application::type_vec_files& files,
				const Glib::ustring& hint) override;

			std::string sock_broadcast_ep;


		private:
			class PoolManagerAppWindow* create_appwindow();
			void on_hide_window(Gtk::Window* window);
			void on_action_quit();
			void on_action_preferences();

		public:
			zmq::socket_t sock_broadcast;



	};

}
