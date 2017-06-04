#include "pool_browser_part.hpp"
#include "pool.hpp"
#include <set>

namespace horizon {
	PoolBrowserPart::PoolBrowserPart(BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>& x, Pool *p, const UUID &e_uuid) :
		Gtk::Box(cobject),
		pool(p), entity_uuid(e_uuid){
		x->get_widget("treeview", w_treeview);
		x->get_widget("MPN_entry", w_MPN_entry);
		x->get_widget("manufacturer_entry", w_manufacturer_entry);
		x->get_widget("tags_entry", w_tags_entry);

		store = Gtk::ListStore::create(list_columns);
		auto view = w_treeview;
		view->set_model(store);
		view->append_column("MPN", list_columns.MPN);
		view->append_column("Manufacturer", list_columns.manufacturer);
		view->append_column("Package", list_columns.package);
		view->append_column("Tags", list_columns.tags);
		view->get_selection()->set_mode(Gtk::SelectionMode::SELECTION_BROWSE);

		sort_controller = std::make_unique<SortController>(view);
		sort_controller->set_simple(true);
		sort_controller->add_column(0, "parts.MPN");
		sort_controller->add_column(1, "parts.manufacturer");
		sort_controller->add_column(2, "packages.name");
		sort_controller->set_sort(0, SortController::Sort::ASC);
		sort_controller->signal_changed().connect(sigc::mem_fun(this, &PoolBrowserPart::search));


		search();

		dynamic_cast<Gtk::CellRendererText*>(view->get_column_cell_renderer(3))->property_ellipsize() = Pango::ELLIPSIZE_END;
		view->get_selection()->set_mode(Gtk::SelectionMode::SELECTION_BROWSE);
		view->signal_row_activated().connect(sigc::mem_fun(this, &PoolBrowserPart::row_activated));
		view->get_selection()->signal_changed().connect(sigc::mem_fun(this, &PoolBrowserPart::selection_changed));

		w_MPN_entry->signal_changed().connect(sigc::mem_fun(this, &PoolBrowserPart::search));
		w_manufacturer_entry->signal_changed().connect(sigc::mem_fun(this, &PoolBrowserPart::search));
		w_tags_entry->signal_changed().connect(sigc::mem_fun(this, &PoolBrowserPart::search));
	}

	void PoolBrowserPart::search() {
		store->freeze_notify();
		store->clear();
		Gtk::TreeModel::Row row;
		std::string MPN_search = w_MPN_entry->get_text();
		std::string manufacturer_search = w_manufacturer_entry->get_text();

		std::istringstream iss(w_tags_entry->get_text());
		std::set<std::string> tags{std::istream_iterator<std::string>{iss}, std::istream_iterator<std::string>{}};
		std::stringstream query;
		if(tags.size() == 0) {
			query << "SELECT parts.uuid, parts.MPN, parts.manufacturer, packages.name, GROUP_CONCAT(tags.tag, ' ') FROM parts LEFT JOIN tags ON tags.uuid = parts.uuid LEFT JOIN packages ON packages.uuid = parts.package WHERE parts.MPN LIKE ? AND parts.manufacturer LIKE ? AND (parts.entity=? or ?) GROUP BY parts.uuid ";
		}
		else {
			query << "SELECT parts.uuid, parts.MPN, parts.manufacturer, packages.name, (SELECT GROUP_CONCAT(tags.tag, ' ') FROM tags WHERE tags.uuid = parts.uuid) FROM parts LEFT JOIN tags ON tags.uuid = parts.uuid LEFT JOIN packages ON packages.uuid = parts.package WHERE parts.MPN LIKE ? AND parts.manufacturer LIKE ? AND (parts.entity=? or ?) ";
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
		}
		store->thaw_notify();
	}

	void PoolBrowserPart::row_activated(const Gtk::TreeModel::Path& path, Gtk::TreeViewColumn* column) {
		auto it = store->get_iter(path);
		if(it) {
			s_signal_part_activated.emit();
		}
	}

	void PoolBrowserPart::selection_changed() {
		auto it = w_treeview->get_selection()->get_selected();
		if(it) {
			Gtk::TreeModel::Row row = *it;
			UUID uu = row[list_columns.uuid];
			std::cout << "selected " << (std::string)uu << std::endl;
			s_signal_part_selected.emit();
		}
	}

	UUID PoolBrowserPart::get_selected_part() {
		auto it = w_treeview->get_selection()->get_selected();
		if(it) {
			Gtk::TreeModel::Row row = *it;
			return row[list_columns.uuid];
		}
		return UUID();
	}

	void PoolBrowserPart::set_MPN(const std::string &s) {
		w_MPN_entry->set_text(s);
		search();
	}

	void PoolBrowserPart::set_show_none(bool v) {
		show_none = v;
		search();
	}

	PoolBrowserPart *PoolBrowserPart::create(class Pool *p, const UUID &e_uuid) {
		PoolBrowserPart* w;
		Glib::RefPtr<Gtk::Builder> x = Gtk::Builder::create();
		x->add_from_resource("/net/carrotIndustries/horizon/widgets/pool_browser_part.ui");
		x->get_widget_derived("browser_box", w, p, e_uuid);
		w->reference();
		return w;
	}
}
