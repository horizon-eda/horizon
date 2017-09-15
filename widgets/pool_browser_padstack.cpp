#include "pool_browser_padstack.hpp"
#include "pool.hpp"
#include <set>

namespace horizon {
	PoolBrowserPadstack::PoolBrowserPadstack(Pool *p): PoolBrowser(p) {
		construct();
		name_entry = create_search_entry("Name");
		search();
	}

	Glib::RefPtr<Gtk::ListStore> PoolBrowserPadstack::create_list_store() {
		return Gtk::ListStore::create(list_columns);
	}

	void PoolBrowserPadstack::create_columns() {
		treeview->append_column("Padstack", list_columns.padstack_name);
		treeview->append_column("Type", list_columns.padstack_type);
		treeview->append_column("Package", list_columns.package_name);
	}

	void PoolBrowserPadstack::add_sort_controller_columns() {
		sort_controller->add_column(0, "padstacks.name");
	}

	void PoolBrowserPadstack::set_package_uuid(const UUID &uu) {
		package_uuid = uu;
		search();
	}

	void PoolBrowserPadstack::search() {
		auto selected_uuid = get_selected();
		store->freeze_notify();
		store->clear();

		std::string name_search = name_entry->get_text();

		SQLite::Query q(pool->db, "SELECT padstacks.uuid, padstacks.name, padstacks.type, packages.name FROM padstacks LEFT JOIN packages ON padstacks.package = packages.uuid WHERE (packages.uuid=? OR ? OR (padstacks.package = '00000000-0000-0000-0000-000000000000' AND padstacks.type NOT IN ('via', 'mechanical'))) AND padstacks.name LIKE ?"+sort_controller->get_order_by());
		q.bind(1, package_uuid);
		q.bind(2, package_uuid==UUID());
		q.bind(3, "%"+name_search+"%");

		Gtk::TreeModel::Row row;
		if(show_none) {
			row = *(store->append());
			row[list_columns.uuid] = UUID();
			row[list_columns.padstack_name] = "none";
		}

		while(q.step()) {
			row = *(store->append());
			row[list_columns.uuid] = q.get<std::string>(0);
			row[list_columns.padstack_name] = q.get<std::string>(1);
			row[list_columns.padstack_type] = q.get<std::string>(2);
			row[list_columns.package_name] = q.get<std::string>(3);
		}
		store->thaw_notify();
		select_uuid(selected_uuid);
		scroll_to_selection();
	}

	UUID PoolBrowserPadstack::uuid_from_row(const Gtk::TreeModel::Row &row) {
		return row[list_columns.uuid];
	}
}
