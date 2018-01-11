#include "pool_browser_padstack.hpp"
#include "pool/pool.hpp"
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
		{
			auto cr = Gtk::manage(new Gtk::CellRendererText());
			auto tvc = Gtk::manage(new Gtk::TreeViewColumn("Path", *cr));
			tvc->add_attribute(cr->property_text(), list_columns.path);
			cr->property_ellipsize() = Pango::ELLIPSIZE_START;
			path_column = treeview->append_column(*tvc)-1;
		}
	}

	void PoolBrowserPadstack::add_sort_controller_columns() {
		sort_controller->add_column(0, "padstacks.name");
	}

	void PoolBrowserPadstack::set_package_uuid(const UUID &uu) {
		package_uuid = uu;
		search();
	}

	void PoolBrowserPadstack::set_include_padstack_type(Padstack::Type ty, bool v) {
		if(padstacks_included.count(ty)) {
			if(!v)
				padstacks_included.erase(ty);
		}
		if(v)
			padstacks_included.insert(ty);
		search();
	}

	void PoolBrowserPadstack::search() {
		auto selected_uuid = get_selected();
		treeview->unset_model();
		store->clear();

		std::string name_search = name_entry->get_text();

		SQLite::Query q(pool->db, "SELECT padstacks.uuid, padstacks.name, padstacks.type, packages.name, padstacks.filename FROM padstacks LEFT JOIN packages ON padstacks.package = packages.uuid WHERE (packages.uuid=? OR ? OR padstacks.package = '00000000-0000-0000-0000-000000000000')  AND padstacks.name LIKE ? AND padstacks.type IN (?, ?, ?, ?, ?, ?) "+sort_controller->get_order_by());
		q.bind(1, package_uuid);
		q.bind(2, package_uuid==UUID());
		q.bind(3, "%"+name_search+"%");
		if(padstacks_included.count(Padstack::Type::TOP))
			q.bind(4, std::string("top"));
		else
			q.bind(4, std::string(""));

		if(padstacks_included.count(Padstack::Type::BOTTOM))
			q.bind(5, std::string("bottom"));
		else
			q.bind(5, std::string(""));

		if(padstacks_included.count(Padstack::Type::THROUGH))
			q.bind(6, std::string("through"));
		else
			q.bind(6, std::string(""));

		if(padstacks_included.count(Padstack::Type::MECHANICAL))
			q.bind(7, std::string("mechanical"));
		else
			q.bind(7, std::string(""));

		if(padstacks_included.count(Padstack::Type::VIA))
			q.bind(8, std::string("via"));
		else
			q.bind(8, std::string(""));

		if(padstacks_included.count(Padstack::Type::HOLE))
			q.bind(9, std::string("hole"));
		else
			q.bind(9, std::string(""));


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
			row[list_columns.path] = q.get<std::string>(4);
		}
		treeview->set_model(store);
		select_uuid(selected_uuid);
		scroll_to_selection();
	}

	UUID PoolBrowserPadstack::uuid_from_row(const Gtk::TreeModel::Row &row) {
		return row[list_columns.uuid];
	}
}
