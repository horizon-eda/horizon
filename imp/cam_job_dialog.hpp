#pragma once
#include <gtkmm.h>
#include <array>
#include <set>
#include "common.hpp"
#include "uuid.hpp"
#include "core/core_board.hpp"
#include "export_gerber/cam_job.hpp"
#include <experimental/optional>
namespace horizon {

	class CAMJobWindow: public Gtk::Window{
		public:
			CAMJobWindow(BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>& x);
			static CAMJobWindow* create(Gtk::Window *p, CoreBoard *c);
		private :
			CoreBoard *core;

			Gtk::FileChooserButton *w_job_file = nullptr;
			Gtk::FileChooserButton *w_output_dir = nullptr;
			Gtk::Entry *w_prefix = nullptr;
			Gtk::TextView *w_output = nullptr;
			Gtk::Label *w_job_title = nullptr;
			Gtk::Button *w_run_button= nullptr;


			std::experimental::optional<CAMJob> job;
			void try_load_job_file();
			void update_can_run();
			void run_job();
	};
}
