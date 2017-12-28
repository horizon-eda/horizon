#include "pool-mgr-app_win.hpp"
#include "pool-mgr-app.hpp"
#include <iostream>
#include "util.hpp"
#include "pool_notebook.hpp"
#include "pool-update/pool-update.hpp"
#include "util/github_client.hpp"
#include <thread>
#include <git2.h>
#include <git2/clone.h>
#include "util/gtk_util.hpp"

extern const char *gitversion;

namespace horizon {
	class PoolRecentItemBox : public Gtk::Box {
		public:
			PoolRecentItemBox(BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>& x, const std::string &filename, const Glib::DateTime &last_opened);
			static PoolRecentItemBox* create(const std::string &filename, const Glib::DateTime &last_opened);
			const std::string filename;

		private :
			Gtk::Label *name_label = nullptr;
			Gtk::Label *path_label = nullptr;
			Gtk::Label *last_opened_label = nullptr;
	};

	PoolRecentItemBox::PoolRecentItemBox(BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>& x, const std::string &fn, const Glib::DateTime &last_opened) :
		Gtk::Box(cobject), filename(fn) {
		x->get_widget("recent_item_name_label", name_label);
		x->get_widget("recent_item_path_label", path_label);
		x->get_widget("recent_item_last_opened_label", last_opened_label);

		path_label->set_text(filename);
		try {
			std::ifstream ifs(filename);
			if(ifs.is_open()) {
				json k;
				ifs>>k;
				name_label->set_text(k.at("name").get<std::string>());
			}
			ifs.close();
		}
		catch (...) {
			name_label->set_text("error opening!");
		}

		last_opened_label->set_text(last_opened.format("%c"));

	}

	PoolRecentItemBox* PoolRecentItemBox::create(const std::string &filename, const Glib::DateTime &last_opened) {
		PoolRecentItemBox* w;
		Glib::RefPtr<Gtk::Builder> x = Gtk::Builder::create();
		x->add_from_resource("/net/carrotIndustries/horizon/pool-mgr/window.ui", "recent_item");
		x->get_widget_derived("recent_item", w, filename, last_opened);
		w->reference();
		return w;
	}

	PoolManagerAppWindow::PoolManagerAppWindow(BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>& refBuilder, PoolManagerApplication *app):
			Gtk::ApplicationWindow(cobject), builder(refBuilder), state_store(this, "pool-mgr"), zctx(app->zctx) {
		builder->get_widget("stack", stack);
		builder->get_widget("button_open", button_open);
		builder->get_widget("button_close", button_close);
		builder->get_widget("button_update", button_update);
		builder->get_widget("spinner_update", spinner_update);
		builder->get_widget("button_download", button_download);
		builder->get_widget("button_do_download", button_do_download);
		builder->get_widget("button_cancel", button_cancel);
		builder->get_widget("header", header);
		builder->get_widget("recent_listbox", recent_listbox);
		builder->get_widget("label_gitversion", label_gitversion);
		builder->get_widget("pool_box", pool_box);
		builder->get_widget("pool_update_status_label", pool_update_status_label);
		builder->get_widget("pool_update_status_rev", pool_update_status_rev);
		builder->get_widget("pool_update_status_close_button", pool_update_status_close_button);
		builder->get_widget("pool_update_progress", pool_update_progress);
		builder->get_widget("download_revealer", download_revealer);
		builder->get_widget("download_label", download_label);
		builder->get_widget("download_gh_repo_entry", download_gh_repo_entry);
		builder->get_widget("download_gh_username_entry", download_gh_username_entry);
		builder->get_widget("download_dest_dir_button", download_dest_dir_button);
		set_view_mode(ViewMode::OPEN);

		button_open->signal_clicked().connect(sigc::mem_fun(this, &PoolManagerAppWindow::handle_open));
		button_close->signal_clicked().connect(sigc::mem_fun(this, &PoolManagerAppWindow::handle_close));
		button_update->signal_clicked().connect(sigc::mem_fun(this, &PoolManagerAppWindow::handle_update));
		button_download->signal_clicked().connect(sigc::mem_fun(this, &PoolManagerAppWindow::handle_download));
		button_do_download->signal_clicked().connect(sigc::mem_fun(this, &PoolManagerAppWindow::handle_do_download));
		button_cancel->signal_clicked().connect(sigc::mem_fun(this, &PoolManagerAppWindow::handle_cancel));
		//recent_chooser->signal_item_activated().connect(sigc::mem_fun(this, &PoolManagerAppWindow::handle_recent));

		pool_update_status_close_button->signal_clicked().connect([this] {
			pool_update_status_rev->set_reveal_child(false);
		});

		label_gitversion->set_text(gitversion);

		set_icon(Gdk::Pixbuf::create_from_resource("/net/carrotIndustries/horizon/icon.svg"));

		download_dispatcher.connect([this]{
			std::lock_guard<std::mutex> lock(download_mutex);
			download_revealer->set_reveal_child(downloading || download_error);
			download_label->set_text(download_status);
			if(!downloading) {
				button_cancel->set_sensitive(true);
				button_do_download->set_sensitive(true);
			}
			if(!downloading && !download_error) {
				open_file_view(Gio::File::create_for_path(Glib::build_filename(download_dest_dir_button->get_filename(), "pool.json")));
			}
		});
		Glib::signal_idle().connect_once([this]{update_recent_items();});

		recent_listbox->set_header_func(sigc::ptr_fun(header_func_separator));
		recent_listbox->signal_row_activated().connect([this](Gtk::ListBoxRow *row) {
			auto ch = dynamic_cast<PoolRecentItemBox*>(row->get_child());
			open_file_view(Gio::File::create_for_path(ch->filename));
		});
	}

	void PoolManagerAppWindow::set_pool_updating(bool v, bool success) {
		button_update->set_sensitive(!v);
		pool_update_status_close_button->set_visible(!success);
		if(success) {
			if(v) { //show immediately
				pool_update_status_rev->set_reveal_child(v);
			}
			else {
				Glib::signal_timeout().connect([this]{
					pool_update_status_rev->set_reveal_child(false);
					return false;
				}, 500);
			}
		}
		if(v)
			spinner_update->start();
		else
			spinner_update->stop();
	}

	void PoolManagerAppWindow::set_pool_update_status_text(const std::string &txt) {
		pool_update_status_label->set_markup("<b>Updating pool:</b> " + txt);
	}

	void PoolManagerAppWindow::set_pool_update_progress(float progress) {
		if(progress < 0) {
			pool_update_progress->pulse();
		}
		else {
			pool_update_progress->set_fraction(progress);
		}
	}

	PoolManagerAppWindow::~PoolManagerAppWindow() {

	}

	void PoolManagerAppWindow::handle_recent() {
		//auto file = Gio::File::create_for_uri(recent_chooser->get_current_uri());
		//open_file_view(file);
	}

	void PoolManagerAppWindow::handle_download() {
		set_view_mode(ViewMode::DOWNLOAD);
	}

	void PoolManagerAppWindow::handle_cancel() {
		set_view_mode(ViewMode::OPEN);
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

	void PoolManagerAppWindow::update_recent_items() {
		auto app = Glib::RefPtr<PoolManagerApplication>::cast_dynamic(get_application());
		if(!app)
			return;
		{
			auto chs = recent_listbox->get_children();
			for(auto it: chs) {
				delete it;
			}
		}
		std::vector<std::pair<std::string, Glib::DateTime>> recent_items_sorted;

		recent_items_sorted.reserve(app->recent_items.size());
		for(const auto &it: app->recent_items) {
			recent_items_sorted.emplace_back(it.first, it.second);
		}
		std::sort(recent_items_sorted.begin(), recent_items_sorted.end(), [](const auto &a, const auto &b){return a.second.to_unix() > b.second.to_unix();});

		for(const auto &it: recent_items_sorted) {
			auto box = PoolRecentItemBox::create(it.first, it.second);
			recent_listbox->append(*box);
			box->show();
			box->unreference();
		}

	}

	void PoolManagerAppWindow::handle_do_download() {

		std::string dest_dir = download_dest_dir_button->get_filename();
		if(dest_dir.size()) {
			button_cancel->set_sensitive(false);
			button_do_download->set_sensitive(false);
			download_error = false;
			download_revealer->set_reveal_child(true);
			downloading = true;
			std::thread dl_thread(&PoolManagerAppWindow::download_thread, this, download_gh_username_entry->get_text(), download_gh_repo_entry->get_text(), dest_dir);
			dl_thread.detach();
		}
		else {
			Gtk::MessageDialog md(*this,  "Destination directory needs to be set", false /* use_markup */, Gtk::MESSAGE_ERROR, Gtk::BUTTONS_OK);
			md.run();
		}
	}

	void PoolManagerAppWindow::download_thread(std::string gh_username,  std::string gh_repo, std::string dest_dir) {
		try {

			{
				Glib::Dir dd(dest_dir);
				for(const auto &it: dd) {
					throw std::runtime_error("destination dir is not empty");
				}
			}

			{
				std::lock_guard<std::mutex> lock(download_mutex);
				download_status = "Fetching clone URL...";
			}
			download_dispatcher.emit();

			GitHubClient client;
			json repo = client.get_repo(gh_username, gh_repo);
			std::string clone_url = repo.at("clone_url");
			std::cout << clone_url << std::endl;

			git_repository *cloned_repo = NULL;
			git_clone_options clone_opts = GIT_CLONE_OPTIONS_INIT;
			git_checkout_options checkout_opts = GIT_CHECKOUT_OPTIONS_INIT;

			checkout_opts.checkout_strategy = GIT_CHECKOUT_SAFE;
			clone_opts.checkout_opts = checkout_opts;

			{
				std::lock_guard<std::mutex> lock(download_mutex);
				download_status = "Cloning repository...";
			}
			download_dispatcher.emit();

			auto remote_dir = Glib::build_filename(dest_dir, ".remote");
			Gio::File::create_for_path(remote_dir)->make_directory_with_parents();
			int error = git_clone(&cloned_repo, clone_url.c_str(), remote_dir.c_str(), &clone_opts);

			if(error != 0) {
				auto gerr = giterr_last();
				throw std::runtime_error("git error " + std::to_string(gerr->klass) +  " " + std::string(gerr->message));
			}

			{
				std::lock_guard<std::mutex> lock(download_mutex);
				download_status = "Updating remote pool...";
			}
			download_dispatcher.emit();

			pool_update(remote_dir);
			Pool pool(remote_dir);

			{
				std::lock_guard<std::mutex> lock(download_mutex);
				download_status = "Copying...";
			}
			download_dispatcher.emit();

			SQLite::Query q(pool.db, "SELECT filename FROM all_items_view");
			while(q.step()) {
				std::string filename = q.get<std::string>(0);
				auto dirname = Glib::build_filename(dest_dir, Glib::path_get_dirname(filename));
				if(!Glib::file_test(dirname, Glib::FILE_TEST_IS_DIR)) {
					Gio::File::create_for_path(dirname)->make_directory_with_parents();
				}
				Gio::File::create_for_path(Glib::build_filename(remote_dir, filename))->copy(Gio::File::create_for_path(Glib::build_filename(dest_dir, filename)));
			}
			Gio::File::create_for_path(Glib::build_filename(remote_dir, "pool.json"))->copy(Gio::File::create_for_path(Glib::build_filename(dest_dir, "pool.json")));
			Gio::File::create_for_path(Glib::build_filename(dest_dir, "tmp"))->make_directory();

			{
				std::lock_guard<std::mutex> lock(download_mutex);
				download_status = "Updating local pool...";
			}
			download_dispatcher.emit();

			pool_update(dest_dir);

			git_repository_free(cloned_repo);


			{
				std::lock_guard<std::mutex> lock(download_mutex);
				download_error = false;
				downloading = false;
			}
			download_dispatcher.emit();
		}
		catch(const std::exception &e) {
			{
				std::lock_guard<std::mutex> lock(download_mutex);
				downloading = false;
				download_error = true;
				download_status = "Error: "+std::string(e.what());
			}
			download_dispatcher.emit();
		}
		catch(const Gio::Error &e) {
			{
				std::lock_guard<std::mutex> lock(download_mutex);
				downloading = false;
				download_error = true;
				download_status = "Error: "+std::string(e.what());
			}
			download_dispatcher.emit();
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
		button_download->hide();
		button_do_download->hide();
		button_cancel->hide();
		header->set_subtitle("");
		header->set_show_close_button(true);

		switch(mode) {
			case ViewMode::OPEN:
				stack->set_visible_child("open");
				button_open->show();
				button_download->show();
				update_recent_items();
			break;

			case ViewMode::POOL:
				stack->set_visible_child("pool");
				button_close->show();
				button_update->show();
			break;

			case ViewMode::DOWNLOAD:
				stack->set_visible_child("download");
				button_cancel->show();
				button_do_download->show();
				header->set_show_close_button(false);
				download_revealer->set_reveal_child(true);
				download_revealer->set_reveal_child(false);
			break;
		}
	}

	PoolManagerAppWindow* PoolManagerAppWindow::create(PoolManagerApplication *app) {
		// Load the Builder file and instantiate its widgets.
		std::vector<Glib::ustring> widgets = {"app_window", "sg_dest", "sg_repo"};
		auto refBuilder = Gtk::Builder::create_from_resource("/net/carrotIndustries/horizon/pool-mgr/window.ui", widgets);

		PoolManagerAppWindow* window = nullptr;
		refBuilder->get_widget_derived("app_window", window, app);

		if (!window)
			throw std::runtime_error("No \"app_window\" object in window.ui");
		return window;
	}



	void PoolManagerAppWindow::open_file_view(const Glib::RefPtr<Gio::File>& file) {
		auto path = file->get_path();
		auto app = Glib::RefPtr<PoolManagerApplication>::cast_dynamic(get_application());
		app->recent_items[path] = Glib::DateTime::create_now_local();
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
