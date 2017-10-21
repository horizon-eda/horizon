#include "pool-mgr-app_win.hpp"
#include "pool-mgr-app.hpp"
#include <iostream>
#include "util.hpp"
#include "pool_notebook.hpp"
#include "pool-update/pool-update.hpp"
extern const char *gitversion;

namespace horizon {

	PoolManagerAppWindow::PoolManagerAppWindow(BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>& refBuilder, PoolManagerApplication *app):
			Gtk::ApplicationWindow(cobject), builder(refBuilder), zctx(app->zctx) {
		builder->get_widget("stack", stack);
		builder->get_widget("button_open", button_open);
		builder->get_widget("button_close", button_close);
		builder->get_widget("button_update", button_update);
		builder->get_widget("spinner_update", spinner_update);
		builder->get_widget("header", header);
		builder->get_widget("recent_chooser", recent_chooser);
		builder->get_widget("label_gitversion", label_gitversion);
		builder->get_widget("pool_box", pool_box);
		set_view_mode(ViewMode::OPEN);

		button_open->signal_clicked().connect(sigc::mem_fun(this, &PoolManagerAppWindow::handle_open));
		button_close->signal_clicked().connect(sigc::mem_fun(this, &PoolManagerAppWindow::handle_close));
		button_update->signal_clicked().connect(sigc::mem_fun(this, &PoolManagerAppWindow::handle_update));
		recent_chooser->signal_item_activated().connect(sigc::mem_fun(this, &PoolManagerAppWindow::handle_recent));

		label_gitversion->set_text(gitversion);

		set_icon(Gdk::Pixbuf::create_from_resource("/net/carrotIndustries/horizon/icon.svg"));
	}

	void PoolManagerAppWindow::set_pool_updating(bool v) {
		button_update->set_sensitive(!v);
		if(v)
			spinner_update->start();
		else
			spinner_update->stop();
	}

	PoolManagerAppWindow::~PoolManagerAppWindow() {

	}

	void PoolManagerAppWindow::handle_recent() {
		auto file = Gio::File::create_for_uri(recent_chooser->get_current_uri());
		open_file_view(file);
	}

	void PoolManagerAppWindow::handle_open() {
		GtkFileChooserNative *native = gtk_file_chooser_native_new ("Open Pool",
			GTK_WINDOW(gobj()),
			GTK_FILE_CHOOSER_ACTION_OPEN,
			"_Open",
			"_Cancel");
		auto chooser = Glib::wrap(GTK_FILE_CHOOSER(native));
		auto filter = Gtk::FileFilter::create();
		filter->set_name("Horizon pools");
		filter->add_pattern("pool.json");
		chooser->add_filter(filter);

		if(gtk_native_dialog_run (GTK_NATIVE_DIALOG (native))==GTK_RESPONSE_ACCEPT) {
			auto file = chooser->get_file();
			open_file_view(file);
		}
	}

	void PoolManagerAppWindow::handle_close() {
		close_pool();
	}

	void PoolManagerAppWindow::handle_update() {
		if(pool_notebook) {
			pool_notebook->pool_update();
		}
	}


	bool PoolManagerAppWindow::close_pool() {
		if(pool_notebook && !pool_notebook->can_close()){
			Gtk::MessageDialog md(*this,  "Can't close right now", false /* use_markup */, Gtk::MESSAGE_ERROR, Gtk::BUTTONS_OK);
			md.set_secondary_text("Close all running editors first");
			md.run();
			return false;
		}
		delete pool_notebook;
		pool_notebook = nullptr;
		set_view_mode(ViewMode::OPEN);
		return true;
	}

	void PoolManagerAppWindow::set_view_mode(ViewMode mode) {
		button_open->hide();
		button_close->hide();
		button_update->hide();
		header->set_subtitle("");

		switch(mode) {
			case ViewMode::OPEN:
				stack->set_visible_child("open");
				button_open->show();
			break;

			case ViewMode::POOL:
				stack->set_visible_child("pool");
				button_close->show();
				button_update->show();

			break;
		}
	}

	PoolManagerAppWindow* PoolManagerAppWindow::create(PoolManagerApplication *app) {
		// Load the Builder file and instantiate its widgets.
		auto refBuilder = Gtk::Builder::create_from_resource("/net/carrotIndustries/horizon/pool-mgr/window.ui");

		PoolManagerAppWindow* window = nullptr;
		refBuilder->get_widget_derived("app_window", window, app);

		if (!window)
			throw std::runtime_error("No \"app_window\" object in window.ui");
		return window;
	}



	void PoolManagerAppWindow::open_file_view(const Glib::RefPtr<Gio::File>& file) {
		auto path = file->get_path();
		try {
			pool_base_path = file->get_parent()->get_path();
			auto pool_db_path = Glib::build_filename(pool_base_path, "pool.db");
			if(!Glib::file_test(pool_db_path, Glib::FILE_TEST_IS_REGULAR)) {
				pool_update(pool_base_path);
			}
			set_view_mode(ViewMode::POOL);
			pool_notebook = new PoolNotebook(pool_base_path, this);
			pool_box->pack_start(*pool_notebook, true, true, 0);
			pool_notebook->show();
		}
		catch (const std::exception& e) {
			Gtk::MessageDialog md(*this,  "Error opening pool", false /* use_markup */, Gtk::MESSAGE_ERROR, Gtk::BUTTONS_OK);
			md.set_secondary_text(e.what());
			md.run();
			close_pool();
		}
	}

	bool PoolManagerAppWindow::on_delete_event(GdkEventAny *ev) {
		//returning false will destroy the window
		std::cout << "close" << std::endl;
		return !close_pool();
	}
}
