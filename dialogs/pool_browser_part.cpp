#include "pool_browser_part.hpp"
#include <iostream>
#include <sqlite3.h>

namespace horizon {


	class PoolBrowserBoxPart: public Gtk::Box {
		public:
			PoolBrowserBoxPart(BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>& x) :
				Gtk::Box(cobject) {
				show_all();
			}
			static PoolBrowserBoxPart* create() {
				PoolBrowserBoxPart* w;
				Glib::RefPtr<Gtk::Builder> x = Gtk::Builder::create();
				x->add_from_resource("/net/carrotIndustries/horizon/dialogs/pool_browser_part.ui");
				x->get_widget_derived("browser_box", w);
				x->get_widget("treeview", w->w_treeview);
				x->get_widget("MPN_entry", w->w_MPN_entry);
				x->get_widget("manufacturer_entry", w->w_manufacturer_entry);
				x->get_widget("tags_entry", w->w_tags_entry);

				w->reference();
				return w;
			}
			Gtk::TreeView *w_treeview;
			Gtk::Entry *w_MPN_entry;
			Gtk::Entry *w_manufacturer_entry;
			Gtk::Entry *w_tags_entry;

		private :
	};



	void PoolBrowserDialogPart::ok_clicked() {
		auto it = box->w_treeview->get_selection()->get_selected();
		if(it) {
			Gtk::TreeModel::Row row = *it;
			selection_valid = true;
			selected_uuid = row[list_columns.uuid];
		}
	}

	void PoolBrowserDialogPart::row_activated(const Gtk::TreeModel::Path& path, Gtk::TreeViewColumn* column) {
		auto it = store->get_iter(path);
		if(it) {
			Gtk::TreeModel::Row row = *it;
			selection_valid = true;
			selected_uuid = row[list_columns.uuid];
			response(Gtk::ResponseType::RESPONSE_OK);
		}
	}

	bool PoolBrowserDialogPart::auto_ok() {
		response(Gtk::ResponseType::RESPONSE_OK);
		return false;
	}

	void PoolBrowserDialogPart::search() {
		store->freeze_notify();
		store->clear();
		Gtk::TreeModel::Row row;
		std::string MPN_search = box->w_MPN_entry->get_text();
		std::string manufacturer_search = box->w_manufacturer_entry->get_text();

		std::istringstream iss(box->w_tags_entry->get_text());
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

		row = *(store->append());
		row[list_columns.uuid] = UUID();
		row[list_columns.MPN] = "none";
		row[list_columns.manufacturer] = "none";
		row[list_columns.package] = "none";

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

	void PoolBrowserDialogPart::set_MPN(const std::string &MPN) {
		box->w_MPN_entry->set_text(MPN);
		search();
	}

	PoolBrowserDialogPart::PoolBrowserDialogPart(Gtk::Window *parent, Pool *ipool, const UUID &ientity_uuid) :
		Gtk::Dialog("Select Part", *parent, Gtk::DialogFlags::DIALOG_MODAL|Gtk::DialogFlags::DIALOG_USE_HEADER_BAR),
		pool(ipool),
		entity_uuid(ientity_uuid){

		Gtk::Button *button_ok = add_button("OK", Gtk::ResponseType::RESPONSE_OK);
		add_button("Cancel", Gtk::ResponseType::RESPONSE_CANCEL);
		set_default_response(Gtk::ResponseType::RESPONSE_OK);
		set_default_size(300, 300);

		button_ok->signal_clicked().connect(sigc::mem_fun(this, &PoolBrowserDialogPart::ok_clicked));

		store = Gtk::ListStore::create(list_columns);


		box = PoolBrowserBoxPart::create();
		get_content_area()->pack_start(*box, true, true, 0);
		get_content_area()->set_border_width(0);
		box->unreference();



		auto view = box->w_treeview;
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
		sort_controller->signal_changed().connect(sigc::mem_fun(this, &PoolBrowserDialogPart::search));


		search();

		dynamic_cast<Gtk::CellRendererText*>(view->get_column_cell_renderer(3))->property_ellipsize() = Pango::ELLIPSIZE_END;
		view->get_selection()->set_mode(Gtk::SelectionMode::SELECTION_BROWSE);
		view->signal_row_activated().connect(sigc::mem_fun(this, &PoolBrowserDialogPart::row_activated));

		box->w_MPN_entry->signal_changed().connect(sigc::mem_fun(this, &PoolBrowserDialogPart::search));
		box->w_manufacturer_entry->signal_changed().connect(sigc::mem_fun(this, &PoolBrowserDialogPart::search));
		box->w_tags_entry->signal_changed().connect(sigc::mem_fun(this, &PoolBrowserDialogPart::search));

		show_all();

	}
}
