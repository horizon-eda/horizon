#include "pool_browser_unit.hpp"
#include "pool.hpp"

namespace horizon {
	PoolBrowserUnit::PoolBrowserUnit(Pool *p): PoolBrowser(p) {
		construct();
		name_entry = create_search_entry("Name");
		search();
	}

	Glib::RefPtr<Gtk::ListStore> PoolBrowserUnit::create_list_store() {
		return Gtk::ListStore::create(list_columns);
	}

	void PoolBrowserUnit::create_columns() {
		treeview->append_column("Unit", list_columns.name);
		treeview->append_column("Manufacturer", list_columns.manufacturer);
	}

	void PoolBrowserUnit::add_sort_controller_columns() {
		sort_controller->add_column(0, "units.name");
		sort_controller->add_column(1, "units.manufacturer");
	}

	void PoolBrowserUnit::search() {
		auto selected_uuid = get_selected();
		store->freeze_notify();
		store->clear();

		std::string name_search = name_entry->get_text();

		std::string query  = "SELECT units.uuid, units.name, units.manufacturer FROM units WHERE units.name LIKE ?"+sort_controller->get_order_by();
		SQLite::Query q(pool->db, query);
		q.bind(1, "%"+name_search+"%");

		Gtk::TreeModel::Row row;
		if(show_none) {
			row = *(store->append());
			row[list_columns.uuid] = UUID();
			row[list_columns.name] = "none";
		}

		while(q.step()) {
			row = *(store->append());
			row[list_columns.uuid] = q.get<std::string>(0);
			row[list_columns.name] = q.get<std::string>(1);
			row[list_columns.manufacturer] = q.get<std::string>(2);
		}
		store->thaw_notify();
		select_uuid(selected_uuid);
		scroll_to_selection();
	}

	UUID PoolBrowserUnit::uuid_from_row(const Gtk::TreeModel::Row &row) {
		return row[list_columns.uuid];
	}
}
