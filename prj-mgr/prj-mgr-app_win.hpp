#pragma once
#include <gtkmm.h>
#include "project/project.hpp"
#include "editor_process.hpp"
#include <memory>

namespace horizon {

	class ProjectManagerViewCreate: public sigc::trackable {
		public:
			ProjectManagerViewCreate(const Glib::RefPtr<Gtk::Builder>& refBuilder);
			void clear();
			std::pair<bool, std::string> create();
			typedef sigc::signal<void, bool> type_signal_valid_change;
			type_signal_valid_change signal_valid_change() {return s_signal_valid_change;}
			void populate_pool_combo(const Glib::RefPtr<Gtk::Application> &app);

		private:
			Gtk::Entry *project_name_entry = nullptr;
			Gtk::Entry *project_description_entry = nullptr;
			Gtk::FileChooserButton *project_path_chooser = nullptr;
			Gtk::Entry *project_dir_entry = nullptr;
			Gtk::ComboBoxText *project_pool_combo = nullptr;
			void update();

			type_signal_valid_change s_signal_valid_change;
	};

	class ProjectManagerViewProject: public sigc::trackable {
		public:
			ProjectManagerViewProject(const Glib::RefPtr<Gtk::Builder>& refBuilder, class ProjectManagerAppWindow *w);
			Gtk::Entry *entry_project_title = nullptr;
			Gtk::Label *label_pool_name = nullptr;
			Gtk::InfoBar *info_bar = nullptr;
			Gtk::Label *info_bar_label = nullptr;

		private:
			ProjectManagerAppWindow *win = nullptr;
			Gtk::Button *button_top_schematic = nullptr;
			Gtk::Button *button_board = nullptr;

			void handle_button_top_schematic();
			void handle_button_board();


	};

	class ProjectManagerProcess {
		public:
			enum class Type {IMP_SCHEMATIC, IMP_BOARD, IMP_PADSTACK};
			ProjectManagerProcess(Type ty, const std::vector<std::string>& args, const std::vector<std::string>& env);
			Type type;
			std::unique_ptr<EditorProcess> proc=nullptr;

	};

	class ProjectManagerAppWindow : public Gtk::ApplicationWindow {
		friend class ProjectManagerViewProject;
		public:
			ProjectManagerAppWindow(BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>& refBuilder);

			static ProjectManagerAppWindow* create();

			void open_file_view(const Glib::RefPtr<Gio::File>& file);
			void spawn_imp(ProjectManagerProcess::Type type, const UUID &pool_uuid, const std::vector<std::string> &args);
		protected:
			Glib::RefPtr<Gtk::Builder> builder;
			Gtk::Stack *stack = nullptr;
			Gtk::Button *button_open = nullptr;
			Gtk::Button *button_new = nullptr;
			Gtk::Button *button_close = nullptr;
			Gtk::Button *button_cancel = nullptr;
			Gtk::Button *button_create = nullptr;
			Gtk::Button *button_save = nullptr;
			Gtk::HeaderBar *header = nullptr;
			Gtk::RecentChooserWidget *recent_chooser = nullptr;
			Gtk::Label *label_gitversion = nullptr;

			std::unique_ptr<Project> project= nullptr;
			std::string project_filename;
			std::map<std::string, ProjectManagerProcess> processes;


			enum class ViewMode {OPEN, PROJECT, CREATE};
			void set_view_mode(ViewMode mode);

			void handle_open();
			void handle_new();
			void handle_cancel();
			void handle_create();
			void handle_close();
			void handle_save();
			void handle_recent();
			ProjectManagerViewCreate view_create;
			ProjectManagerViewProject view_project;

			bool close_project();
			bool on_delete_event(GdkEventAny *ev) override;
			bool check_pools();

	};
};




