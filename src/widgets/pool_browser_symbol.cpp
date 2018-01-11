#include "pool_browser_symbol.hpp"
#include "pool/pool.hpp"

namespace horizon {
	PoolBrowserSymbol::PoolBrowserSymbol(Pool *p, const UUID &uu): PoolBrowser(p), unit_uuid(uu) {
		construct();
		name_entry = create_search_entry("Name");
		search();
	}

	Glib::RefPtr<Gtk::ListStore> PoolBrowserSymbol::create_list_store() {
		return Gtk::ListStore::create(list_columns);
	}

	void PoolBrowserSymbol::create_columns() {
		treeview->append_column("Symbol", list_columns.name);
		treeview->append_column("Unit", list_columns.unit_name);
		treeview->append_column("Manufacturer", list_columns.unit_manufacturer);
	}

	void PoolBrowserSymbol::add_sort_controller_columns() {
		sort_controller->add_column(0, "symbols.name");
		sort_controller->add_column(1, "units.name");
		sort_controller->add_column(2, "units.manufacturer");
		{
			auto cr = Gtk::manage(new Gtk::CellRendererText());
			auto tvc = Gtk::manage(new Gtk::TreeViewColumn("Path", *cr));
			tvc->add_attribute(cr->property_text(), list_columns.path);
			cr->property_ellipsize() = Pango::ELLIPSIZE_START;
			path_column = treeview->append_column(*tvc)-1;
		}
	}

	void PoolBrowserSymbol::set_unit_uuid(const UUID &uu) {
		unit_uuid = uu;
		search();
	}

	void PoolBrowserSymbol::search() {
		auto selected_uuid = get_selected();
		treeview->unset_model();
		store->clear();

		std::string name_search = name_entry->get_text();

		std::string query  = "SELECT symbols.uuid, symbols.name, units.name, units.manufacturer, symbols.filename FROM symbols,units WHERE symbols.unit = units.uuid AND (units.uuid=? OR ?) AND symbols.name LIKE ?"+sort_controller->get_order_by();
		SQLite::Query q(pool->db, query);
		q.bind(1, unit_uuid);
		q.bind(2, unit_uuid==UUID());
		q.bind(3, "%"+name_search+"%");

		Gtk::TreeModel::Row row;
		if(show_none) {
			row = *(store->append());
			row[list_columns.uuid] = UUID();
			row[list_columns.name] = "none";
			row[list_columns.unit_name] = "none";
		}

		while(q.step()) {
			row = *(store->append());
			row[list_columns.uuid] = q.get<std::string>(0);
			row[list_columns.name] = q.get<std::string>(1);
			row[list_columns.unit_name] = q.get<std::string>(2);
			row[list_columns.unit_manufacturer] = q.get<std::string>(3);
			row[list_columns.path] = q.get<std::string>(4);
		}
		treeview->set_model(store);
		select_uuid(selected_uuid);
		scroll_to_selection();
	}

	UUID PoolBrowserSymbol::uuid_from_row(const Gtk::TreeModel::Row &row) {
		return row[list_columns.uuid];
	}
}
