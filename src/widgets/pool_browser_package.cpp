#include "pool_browser_package.hpp"
#include "pool/pool.hpp"
#include <set>

namespace horizon {
	PoolBrowserPackage::PoolBrowserPackage(Pool *p): PoolBrowser(p) {
		construct();
		name_entry = create_search_entry("Name");
		tags_entry = create_search_entry("Tags");
		search();
	}

	Glib::RefPtr<Gtk::ListStore> PoolBrowserPackage::create_list_store() {
		return Gtk::ListStore::create(list_columns);
	}

	void PoolBrowserPackage::create_columns() {
		treeview->append_column("Package", list_columns.name);
		treeview->append_column("Manufacturer", list_columns.manufacturer);
		treeview->append_column("Pads", list_columns.n_pads);
		treeview->append_column("Tags", list_columns.tags);
		{
			auto cr = Gtk::manage(new Gtk::CellRendererText());
			auto tvc = Gtk::manage(new Gtk::TreeViewColumn("Path", *cr));
			tvc->add_attribute(cr->property_text(), list_columns.path);
			cr->property_ellipsize() = Pango::ELLIPSIZE_START;
			path_column = treeview->append_column(*tvc)-1;
		}
	}

	void PoolBrowserPackage::add_sort_controller_columns() {
		sort_controller->add_column(0, "packages.name");
		sort_controller->add_column(1, "packages.manufacturer");
	}

	void PoolBrowserPackage::search() {
		auto selected_uuid = get_selected();
		treeview->unset_model();
		store->clear();
		Gtk::TreeModel::Row row;
		std::string name_search = name_entry->get_text();

		std::istringstream iss(tags_entry->get_text());
		std::set<std::string> tags{std::istream_iterator<std::string>{iss}, std::istream_iterator<std::string>{}};
		std::string query;
		if(tags.size() == 0) {
			query = "SELECT packages.uuid, packages.name, packages.manufacturer,  packages.n_pads, GROUP_CONCAT(tags.tag, ' '), packages.filename FROM packages LEFT JOIN tags ON tags.uuid = packages.uuid WHERE packages.name LIKE ? GROUP by packages.uuid ORDER BY name";
		}
		else {
			std::ostringstream qs;
			qs << "SELECT packages.uuid, packages.name, packages.manufacturer, packages.n_pads, (SELECT GROUP_CONCAT(tags.tag, ' ') FROM tags WHERE tags.uuid = packages.uuid), packages.filename FROM packages LEFT JOIN tags ON tags.uuid = packages.uuid WHERE packages.name LIKE ? ";
			qs << "AND (";
			for(const auto &it: tags) {
				qs << "tags.tag LIKE ? OR ";
			}
			qs << "0) GROUP by packages.uuid HAVING count(*) >= $ntags";
			qs << sort_controller->get_order_by();
			query = qs.str();
		}
		SQLite::Query q(pool->db, query);
		q.bind(1, "%"+name_search+"%");
		int i = 0;
		for(const auto &it: tags) {
			q.bind(i+2, it+"%");
			i++;
		}
		if(tags.size())
			q.bind("$ntags", tags.size());

		if(show_none) {
			row = *(store->append());
			row[list_columns.uuid] = UUID();
			row[list_columns.name] = "none";
			row[list_columns.manufacturer] = "none";
		}

		while(q.step()) {
			row = *(store->append());
			row[list_columns.uuid] = q.get<std::string>(0);
			row[list_columns.name] = q.get<std::string>(1);
			row[list_columns.manufacturer] = q.get<std::string>(2);
			row[list_columns.n_pads] = q.get<int>(3);
			row[list_columns.tags] = q.get<std::string>(4);
			row[list_columns.path] = q.get<std::string>(5);
		}
		treeview->set_model(store);
		select_uuid(selected_uuid);
		scroll_to_selection();
	}

	UUID PoolBrowserPackage::uuid_from_row(const Gtk::TreeModel::Row &row) {
		return row[list_columns.uuid];
	}
}
