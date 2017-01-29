#include "cam_job_dialog.hpp"
#include "export_gerber/gerber_export.hpp"

namespace horizon {



	CAMJobWindow::CAMJobWindow(BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>& x) :
		Gtk::Window(cobject) {
	}

	CAMJobWindow* CAMJobWindow::create(Gtk::Window *p, CoreBoard *c) {
		CAMJobWindow* w;
		Glib::RefPtr<Gtk::Builder> x = Gtk::Builder::create();
		x->add_from_resource("/net/carrotIndustries/horizon/imp/cam_job.ui");
		x->get_widget_derived("window", w);
		x->get_widget("job_file", w->w_job_file);
		x->get_widget("output_dir", w->w_output_dir);
		x->get_widget("prefix", w->w_prefix);
		x->get_widget("output", w->w_output);
		x->get_widget("job_title", w->w_job_title);
		x->get_widget("run_button", w->w_run_button);

		w->set_transient_for(*p);
		w->core = c;
		w->w_job_file->signal_file_set().connect(sigc::mem_fun(w, &CAMJobWindow::try_load_job_file));
		w->w_output_dir->signal_file_set().connect(sigc::mem_fun(w, &CAMJobWindow::update_can_run));
		w->w_run_button->signal_clicked().connect(sigc::mem_fun(w, &CAMJobWindow::run_job));


		return w;
	}

	void CAMJobWindow::try_load_job_file() {
		std::string job_filename = w_job_file->get_filename();
		try {
			job = CAMJob::new_from_file(job_filename);
			w_job_title->set_text(job->title);
			w_run_button->set_sensitive(true);
		}
		catch (const std::exception& e) {
			w_job_title->set_text("Load error: "+std::string(e.what()));
			w_run_button->set_sensitive(false);
			job = {};
		}
		update_can_run();
	}

	void CAMJobWindow::update_can_run() {
		w_run_button->set_sensitive(job && w_output_dir->get_filename().size()>0);
	}

	void CAMJobWindow::run_job() {
		if(!job)
			return;
		std::string prefix = w_output_dir->get_filename()+"/"+w_prefix->get_text();
		GerberExporter ex(core, *job, prefix);
		ex.save();
		w_output->get_buffer()->set_text(ex.get_log());
	}
}
