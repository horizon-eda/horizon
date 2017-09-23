#pragma once
#include <gtkmm.h>
#include <memory>
#include "uuid.hpp"
#include <zmq.hpp>
#include "json.hpp"
#include "util/editor_process.hpp"

namespace horizon {
	using json = nlohmann::json;

	class PoolManagerAppWindow : public Gtk::ApplicationWindow {
		friend class ProjectManagerViewProject;
		public:
			PoolManagerAppWindow(BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>& refBuilder, class PoolManagerApplication *app);
			~PoolManagerAppWindow();

			static PoolManagerAppWindow* create(class PoolManagerApplication *app);

			void open_file_view(const Glib::RefPtr<Gio::File>& file);
		protected:
			Glib::RefPtr<Gtk::Builder> builder;
			Gtk::Stack *stack = nullptr;
			Gtk::Button *button_open = nullptr;
			Gtk::Button *button_close = nullptr;
			Gtk::Button *button_update = nullptr;
			Gtk::Spinner *spinner_update = nullptr;
			Gtk::HeaderBar *header = nullptr;
			Gtk::RecentChooserWidget *recent_chooser = nullptr;
			Gtk::Label *label_gitversion = nullptr;
			Gtk::Box *pool_box = nullptr;
			class PoolNotebook *pool_notebook = nullptr;

			std::string pool_base_path;

			enum class ViewMode {OPEN, POOL};
			void set_view_mode(ViewMode mode);

			void handle_open();
			void handle_close();
			void handle_recent();
			void handle_update();
			json handle_req(const json &j);

			bool close_pool();
			bool on_delete_event(GdkEventAny *ev) override;

		public:
			zmq::context_t &zctx;
			void set_pool_updating(bool v);

	};
};




