#include "pool_browser_part.hpp"
#include "pool/pool.hpp"
#include <set>

namespace horizon {
	PoolBrowserPart::PoolBrowserPart(Pool *p, const UUID &uu): PoolBrowser(p), entity_uuid(uu) {
		construct();
		MPN_entry = create_search_entry("MPN");
		manufacturer_entry = create_search_entry("Manufacturer");
		tags_entry = create_search_entry("Tags");
		search();
	}

	void PoolBrowserPart::set_MPN(const std::string &s) {
		MPN_entry->set_text(s);
		search();
	}

	void PoolBrowserPart::set_entity_uuid(const UUID &uu) {
		entity_uuid = uu;
		search();
	}

	Glib::RefPtr<Gtk::ListStore> PoolBrowserPart::create_list_store() {
		return Gtk::ListStore::create(list_columns);
	}

	void PoolBrowserPart::create_columns() {
		treeview->append_column("MPN", list_columns.MPN);
		treeview->append_column("Manufacturer", list_columns.manufacturer);
		{
			auto cr = Gtk::manage(new Gtk::CellRendererText());
			auto tvc = Gtk::manage(new Gtk::TreeViewColumn("Description", *cr));
			tvc->set_resizable(true);
			tvc->add_attribute(cr->property_text(), list_columns.description);
			cr->property_ellipsize() = Pango::ELLIPSIZE_END;
			treeview->append_column(*tvc);
		}
		treeview->append_column("Package", list_columns.package);
		treeview->append_column("Tags", list_columns.tags);
		{
			auto cr = Gtk::manage(new Gtk::CellRendererText());
			auto tvc = Gtk::manage(new Gtk::TreeViewColumn("Path", *cr));
			tvc->add_attribute(cr->property_text(), list_columns.path);
			cr->property_ellipsize() = Pango::ELLIPSIZE_START;
			path_column = treeview->append_column(*tvc)-1;
		}
	}

	void PoolBrowserPart::add_sort_controller_columns() {
		sort_controller->add_column(0, "parts.MPN");
		sort_controller->add_column(1, "parts.manufacturer");
		sort_controller->add_column(2, "packages.name");
	}

	void PoolBrowserPart::search() {
		auto selected_uuid = get_selected();
		treeview->unset_model();
		store->clear();
		Gtk::TreeModel::Row row;
		std::string MPN_search = MPN_entry->get_text();
		std::string manufacturer_search = manufacturer_entry->get_text();

		std::istringstream iss(tags_entry->get_text());
		std::set<std::string> tags{std::istream_iterator<std::string>{iss}, std::istream_iterator<std::string>{}};
		std::stringstream query;
		if(tags.size() == 0) {
			query << "SELECT parts.uuid, parts.MPN, parts.manufacturer, packages.name, GROUP_CONCAT(tags.tag, ' '), parts.filename, parts.description FROM parts LEFT JOIN tags ON tags.uuid = parts.uuid LEFT JOIN packages ON packages.uuid = parts.package WHERE parts.MPN LIKE ? AND parts.manufacturer LIKE ? AND (parts.entity=? or ?) GROUP BY parts.uuid ";
		}
		else {
			query << "SELECT parts.uuid, parts.MPN, parts.manufacturer, packages.name, (SELECT GROUP_CONCAT(tags.tag, ' ') FROM tags WHERE tags.uuid = parts.uuid), parts.filename, parts.description FROM parts LEFT JOIN tags ON tags.uuid = parts.uuid LEFT JOIN packages ON packages.uuid = parts.package WHERE parts.MPN LIKE ? AND parts.manufacturer LIKE ? AND (parts.entity=? or ?) ";
			query << "AND (";
			for(const auto &it: tags) {
				query << "tags.tag LIKE ? OR ";
			}
			query << "0) GROUP by parts.uuid HAVING count(*) >= $ntags ";
		}
		query << sort_controller->get_order_by();
		std::cout << query.str() << std::endl;
		SQLite::Query q(pool->db, query.str());
		q.bind(1, "%"+MPN_search+"%");
		q.bind(2, "%"+manufacturer_search+"%");
		q.bind(3, entity_uuid);
		q.bind(4, entity_uuid==UUID());
		int i = 0;
		for(const auto &it: tags) {
			q.bind(i+5, it+"%");
			i++;
		}
		if(tags.size())
			q.bind("$ntags", tags.size());

		if(show_none) {
			row = *(store->append());
			row[list_columns.uuid] = UUID();
			row[list_columns.MPN] = "none";
			row[list_columns.manufacturer] = "none";
			row[list_columns.package] = "none";
		}

		while(q.step()) {
			row = *(store->append());
			row[list_columns.uuid] = q.get<std::string>(0);
			row[list_columns.MPN] = q.get<std::string>(1);
			row[list_columns.manufacturer] = q.get<std::string>(2);
			row[list_columns.package] = q.get<std::string>(3);
			row[list_columns.tags] = q.get<std::string>(4);
			row[list_columns.path] = q.get<std::string>(5);
			row[list_columns.description] = q.get<std::string>(6);
		}
		treeview->set_model(store);
		select_uuid(selected_uuid);
		scroll_to_selection();
	}

	UUID PoolBrowserPart::uuid_from_row(const Gtk::TreeModel::Row &row) {
		return row[list_columns.uuid];
	}
}
