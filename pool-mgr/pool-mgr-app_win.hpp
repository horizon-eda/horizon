#pragma once
#include <gtkmm.h>
#include <memory>
#include "uuid.hpp"
#include <zmq.hpp>
#include "json.hpp"
#include "util/editor_process.hpp"
#include "util/window_state_store.hpp"

namespace horizon {
	using json = nlohmann::json;

	class PoolManagerAppWindow : public Gtk::ApplicationWindow {
		friend class ProjectManagerViewProject;
		public:
			PoolManagerAppWindow(BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>& refBuilder, class PoolManagerApplication *app);
			~PoolManagerAppWindow();

			static PoolManagerAppWindow* create(class PoolManagerApplication *app);

			void open_file_view(const Glib::RefPtr<Gio::File>& file);
			bool close_pool();
		protected:
			Glib::RefPtr<Gtk::Builder> builder;
			Gtk::Stack *stack = nullptr;
			Gtk::Button *button_open = nullptr;
			Gtk::Button *button_close = nullptr;
			Gtk::Button *button_update = nullptr;
			Gtk::Button *button_download = nullptr;
			Gtk::Button *button_do_download = nullptr;
			Gtk::Button *button_cancel = nullptr;
			Gtk::Spinner *spinner_update = nullptr;
			Gtk::Revealer *download_revealer = nullptr;
			Gtk::Label *download_label = nullptr;

			Gtk::Entry *download_gh_username_entry = nullptr;
			Gtk::Entry *download_gh_repo_entry = nullptr;
			Gtk::FileChooserButton *download_dest_dir_button = nullptr;

			Gtk::HeaderBar *header = nullptr;
			Gtk::RecentChooserWidget *recent_chooser = nullptr;
			Gtk::Label *label_gitversion = nullptr;
			Gtk::Box *pool_box = nullptr;
			class PoolNotebook *pool_notebook = nullptr;

			Gtk::Label *pool_update_status_label = nullptr;
			Gtk::Revealer *pool_update_status_rev = nullptr;
			Gtk::Button *pool_update_status_close_button = nullptr;
			Gtk::ProgressBar *pool_update_progress = nullptr;

			std::string pool_base_path;

			enum class ViewMode {OPEN, POOL, DOWNLOAD};
			void set_view_mode(ViewMode mode);

			void handle_open();
			void handle_close();
			void handle_recent();
			void handle_update();
			void handle_download();
			void handle_do_download();
			void handle_cancel();
			json handle_req(const json &j);

			bool on_delete_event(GdkEventAny *ev) override;

			void download_thread(std::string gh_username,  std::string gh_repo, std::string dest_dir);

			Glib::Dispatcher download_dispatcher;

			bool downloading = false;
			bool download_error = false;
			std::string download_status;
			std::mutex download_mutex;



			WindowStateStore state_store;

		public:
			zmq::context_t &zctx;
			void set_pool_updating(bool v, bool success);
			void set_pool_update_status_text(const std::string &txt);
			void set_pool_update_progress(float progress);

	};
};




