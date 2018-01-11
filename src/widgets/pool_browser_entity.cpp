#include "pool_browser_entity.hpp"
#include "pool/pool.hpp"
#include <set>

namespace horizon {
	PoolBrowserEntity::PoolBrowserEntity(Pool *p): PoolBrowser(p) {
		construct();
		name_entry = create_search_entry("Name");
		tags_entry = create_search_entry("Tags");
		search();
	}

	Glib::RefPtr<Gtk::ListStore> PoolBrowserEntity::create_list_store() {
		return Gtk::ListStore::create(list_columns);
	}

	void PoolBrowserEntity::create_columns() {
		treeview->append_column("Entity", list_columns.entity_name);
		treeview->append_column("Manufacturer", list_columns.entity_manufacturer);
		treeview->append_column("Prefix", list_columns.prefix);
		treeview->append_column("Gates", list_columns.n_gates);
		treeview->append_column("Tags", list_columns.tags);
		{
			auto cr = Gtk::manage(new Gtk::CellRendererText());
			auto tvc = Gtk::manage(new Gtk::TreeViewColumn("Path", *cr));
			tvc->add_attribute(cr->property_text(), list_columns.path);
			cr->property_ellipsize() = Pango::ELLIPSIZE_START;
			path_column = treeview->append_column(*tvc)-1;
		}
	}

	void PoolBrowserEntity::add_sort_controller_columns() {
		sort_controller->add_column(0, "entities.name");
		sort_controller->add_column(1, "entities.manufacturer");
	}

	void PoolBrowserEntity::search() {
		auto selected_uuid = get_selected();
		treeview->unset_model();
		store->clear();

		std::string name_search = name_entry->get_text();

		std::istringstream iss(tags_entry->get_text());
		std::set<std::string> tags{std::istream_iterator<std::string>{iss}, std::istream_iterator<std::string>{}};
		std::string query;
		if(tags.size() == 0) {
			query = "SELECT entities.uuid, entities.name, entities.prefix, entities.n_gates, GROUP_CONCAT(tags.tag, ' '), entities.manufacturer, entities.filename FROM entities LEFT JOIN tags ON tags.uuid = entities.uuid WHERE entities.name LIKE ? GROUP by entities.uuid"+sort_controller->get_order_by();
		}
		else {
			std::ostringstream qs;
			qs << "SELECT entities.uuid, entities.name, entities.prefix, entities.n_gates, (SELECT GROUP_CONCAT(tags.tag, ' ') FROM tags WHERE tags.uuid = entities.uuid), entities.manufacturer, entities.filename FROM entities LEFT JOIN tags ON tags.uuid = entities.uuid WHERE entities.name LIKE ? ";
			qs << "AND (";
			for(const auto &it: tags) {
				qs << "tags.tag LIKE ? OR ";
			}
			qs << "0) GROUP by entities.uuid HAVING count(*) >= $ntags"+sort_controller->get_order_by();
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

		Gtk::TreeModel::Row row;
		if(show_none) {
			row = *(store->append());
			row[list_columns.uuid] = UUID();
			row[list_columns.entity_name] = "none";
		}

		while(q.step()) {
			row = *(store->append());
			row[list_columns.uuid] = q.get<std::string>(0);
			row[list_columns.entity_name] = q.get<std::string>(1);
			row[list_columns.prefix] = q.get<std::string>(2);
			row[list_columns.n_gates] = q.get<int>(3);
			row[list_columns.tags] = q.get<std::string>(4);
			row[list_columns.entity_manufacturer] = q.get<std::string>(5);
			row[list_columns.path] = q.get<std::string>(6);
		}
		treeview->set_model(store);
		select_uuid(selected_uuid);
		scroll_to_selection();
	}

	UUID PoolBrowserEntity::uuid_from_row(const Gtk::TreeModel::Row &row) {
		return row[list_columns.uuid];
	}
}
