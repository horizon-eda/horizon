#include "pool_browser_padstack.hpp"
#include <iostream>
#include "sqlite.hpp"

namespace horizon {

	void PoolBrowserDialogPadstack::ok_clicked() {
		auto it = view->get_selection()->get_selected();
		if(it) {
			Gtk::TreeModel::Row row = *it;
			selection_valid = true;
			selected_uuid = row[list_columns.uuid];
		}
	}

	void PoolBrowserDialogPadstack::row_activated(const Gtk::TreeModel::Path& path, Gtk::TreeViewColumn* column) {
		auto it = store->get_iter(path);
		if(it) {
			Gtk::TreeModel::Row row = *it;
			selection_valid = true;
			selected_uuid = row[list_columns.uuid];
			response(Gtk::ResponseType::RESPONSE_OK);
		}
	}

	bool PoolBrowserDialogPadstack::auto_ok() {
		response(Gtk::ResponseType::RESPONSE_OK);
		return false;
	}

	PoolBrowserDialogPadstack::PoolBrowserDialogPadstack(Gtk::Window *parent, Pool *ipool, const UUID &pkg_uuid) :
		Gtk::Dialog("Select Padstack", *parent, Gtk::DialogFlags::DIALOG_MODAL|Gtk::DialogFlags::DIALOG_USE_HEADER_BAR),
		pool(ipool),
		package_uuid(pkg_uuid){
		Gtk::Button *button_ok = add_button("OK", Gtk::ResponseType::RESPONSE_OK);
		add_button("Cancel", Gtk::ResponseType::RESPONSE_CANCEL);
		set_default_response(Gtk::ResponseType::RESPONSE_OK);
		set_default_size(300, 300);

		button_ok->signal_clicked().connect(sigc::mem_fun(this, &PoolBrowserDialogPadstack::ok_clicked));

		store = Gtk::ListStore::create(list_columns);
		Gtk::TreeModel::Row row;
		SQLite::Query q(pool->db, "SELECT padstacks.uuid, padstacks.name FROM padstacks WHERE padstacks.type != 'via' AND (padstacks.package=? OR padstacks.package = '00000000-0000-0000-0000-000000000000')");
		q.bind(1, package_uuid);
		unsigned int n = 0;
		while(q.step()) {
			row = *(store->append());
			row[list_columns.uuid] = q.get<std::string>(0);
			row[list_columns.padstack_name] = q.get<std::string>(1);
			n++;
		}
		/*if(n == 1) {
			selection_valid = true;
			selected_uuid = row[list_columns.uuid];
			Glib::signal_idle().connect( sigc::mem_fun(this, &PoolBrowserDialogSymbol::auto_ok));
			return;
		}*/


		view = Gtk::manage(new Gtk::TreeView(store));
		view->append_column("Padstack Name", list_columns.padstack_name);
		view->get_selection()->set_mode(Gtk::SelectionMode::SELECTION_BROWSE);
		view->signal_row_activated().connect(sigc::mem_fun(this, &PoolBrowserDialogPadstack::row_activated));


		auto sc = Gtk::manage(new Gtk::ScrolledWindow());
		sc->add(*view);
		get_content_area()->pack_start(*sc, true, true, 0);
		get_content_area()->set_border_width(0);

		show_all();
	}
}
