#include "pool_update_error_dialog.hpp"
#include <iostream>

namespace horizon {
	PoolUpdateErrorDialog::PoolUpdateErrorDialog(Gtk::Window *parent, const std::deque<std::tuple<PoolUpdateStatus, std::string, std::string>> &errors) :
		Gtk::Dialog("Pool update errors ", *parent, Gtk::DialogFlags::DIALOG_MODAL|Gtk::DialogFlags::DIALOG_USE_HEADER_BAR) {
		set_default_size(300, 300);

		store = Gtk::ListStore::create(list_columns);
		Gtk::TreeModel::Row row;
		for(auto it: errors) {
			row = *(store->append());
			row[list_columns.filename] = std::get<1>(it);
			row[list_columns.error] = std::get<2>(it);
		}

		view = Gtk::manage(new Gtk::TreeView(store));
		view->append_column("Filename", list_columns.filename);
		view->append_column("Error", list_columns.error);

		auto sc = Gtk::manage(new Gtk::ScrolledWindow());
		sc->set_policy(Gtk::POLICY_NEVER, Gtk::POLICY_AUTOMATIC);
		sc->add(*view);
		get_content_area()->pack_start(*sc, true, true, 0);
		get_content_area()->set_border_width(0);

		show_all();
	}
}
