#include "pool_browser_entity.hpp"
#include "pool_browser_box.hpp"
#include <iostream>
#include "sqlite.hpp"

namespace horizon {

	void PoolBrowserDialogEntity::ok_clicked() {
		auto it = box->w_treeview->get_selection()->get_selected();
		if(it) {
			Gtk::TreeModel::Row row = *it;
			selection_valid = true;
			selected_uuid = row[list_columns.uuid];
		}
	}

	void PoolBrowserDialogEntity::row_activated(const Gtk::TreeModel::Path& path, Gtk::TreeViewColumn* column) {
		auto it = store->get_iter(path);
		if(it) {
			Gtk::TreeModel::Row row = *it;
			selection_valid = true;
			selected_uuid = row[list_columns.uuid];
			response(Gtk::ResponseType::RESPONSE_OK);
		}
	}

	bool PoolBrowserDialogEntity::auto_ok() {
		response(Gtk::ResponseType::RESPONSE_OK);
		return false;
	}

	void PoolBrowserDialogEntity::search() {
		store->freeze_notify();
		store->clear();
		Gtk::TreeModel::Row row;
		std::string name_search = box->w_name_entry->get_text();

		std::istringstream iss(box->w_tags_entry->get_text());
		std::set<std::string> tags{std::istream_iterator<std::string>{iss}, std::istream_iterator<std::string>{}};
		std::string query;
		if(tags.size() == 0) {
			query = "SELECT entities.uuid, entities.name, entities.prefix, entities.n_gates, GROUP_CONCAT(tags.tag, ' ') FROM entities LEFT JOIN tags ON tags.uuid = entities.uuid WHERE entities.name LIKE ? GROUP by entities.uuid ORDER BY name";
		}
		else {
			std::ostringstream qs;
			qs << "SELECT entities.uuid, entities.name, entities.prefix, entities.n_gates, (SELECT GROUP_CONCAT(tags.tag, ' ') FROM tags WHERE tags.uuid = entities.uuid) FROM entities LEFT JOIN tags ON tags.uuid = entities.uuid WHERE entities.name LIKE ? ";
			qs << "AND (";
			for(const auto &it: tags) {
				qs << "tags.tag LIKE ? OR ";
			}
			qs << "0) GROUP by entities.uuid HAVING count(*) >= $ntags ORDER BY name";
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
		while(q.step()) {
			row = *(store->append());
			row[list_columns.uuid] = q.get<std::string>(0);
			row[list_columns.entity_name] = q.get<std::string>(1);
			row[list_columns.prefix] = q.get<std::string>(2);
			row[list_columns.n_gates] = q.get<int>(3);
			row[list_columns.tags] = q.get<std::string>(4);
		}
		store->thaw_notify();
	}

	PoolBrowserDialogEntity::PoolBrowserDialogEntity(Gtk::Window *parent, Pool *ipool) :
		Gtk::Dialog("Select Entity", *parent, Gtk::DialogFlags::DIALOG_MODAL|Gtk::DialogFlags::DIALOG_USE_HEADER_BAR),
		pool(ipool)
		{
		Gtk::Button *button_ok = add_button("OK", Gtk::ResponseType::RESPONSE_OK);
		add_button("Cancel", Gtk::ResponseType::RESPONSE_CANCEL);
		set_default_response(Gtk::ResponseType::RESPONSE_OK);
		set_default_size(300, 300);

		button_ok->signal_clicked().connect(sigc::mem_fun(this, &PoolBrowserDialogEntity::ok_clicked));

		store = Gtk::ListStore::create(list_columns);


		box = PoolBrowserBox::create();
		get_content_area()->pack_start(*box, true, true, 0);
		get_content_area()->set_border_width(0);
		box->unreference();

		search();

		auto view = box->w_treeview;
		view->set_model(store);
		view->append_column("Entity Name", list_columns.entity_name);
		view->append_column("Prefix", list_columns.prefix);
		view->append_column("Gates", list_columns.n_gates);
		view->append_column("Tags", list_columns.tags);
		dynamic_cast<Gtk::CellRendererText*>(view->get_column_cell_renderer(3))->property_ellipsize() = Pango::ELLIPSIZE_END;
		view->get_selection()->set_mode(Gtk::SelectionMode::SELECTION_BROWSE);
		view->signal_row_activated().connect(sigc::mem_fun(this, &PoolBrowserDialogEntity::row_activated));

		box->w_name_entry->signal_changed().connect(sigc::mem_fun(this, &PoolBrowserDialogEntity::search));
		box->w_tags_entry->signal_changed().connect(sigc::mem_fun(this, &PoolBrowserDialogEntity::search));

		show_all();
	}
}
