#include "pool_browser_symbol.hpp"
#include <iostream>
#include "sqlite.hpp"
#include "canvas/canvas.hpp"

namespace horizon {

	void PoolBrowserDialogSymbol::ok_clicked() {
		auto it = view->get_selection()->get_selected();
		if(it) {
			Gtk::TreeModel::Row row = *it;
			selection_valid = true;
			selected_uuid = row[list_columns.uuid];
		}
	}

	void PoolBrowserDialogSymbol::row_activated(const Gtk::TreeModel::Path& path, Gtk::TreeViewColumn* column) {
		auto it = store->get_iter(path);
		if(it) {
			Gtk::TreeModel::Row row = *it;
			selection_valid = true;
			selected_uuid = row[list_columns.uuid];
			response(Gtk::ResponseType::RESPONSE_OK);
		}
	}

	bool PoolBrowserDialogSymbol::auto_ok() {
		response(Gtk::ResponseType::RESPONSE_OK);
		return false;
	}

	PoolBrowserDialogSymbol::PoolBrowserDialogSymbol(Gtk::Window *parent, Core *c, const UUID &iunit_uuid) :
		Gtk::Dialog("Select Symbol", *parent, Gtk::DialogFlags::DIALOG_MODAL|Gtk::DialogFlags::DIALOG_USE_HEADER_BAR),
		pool(c->m_pool),
		core(c),
		unit_uuid(iunit_uuid){
		Gtk::Button *button_ok = add_button("OK", Gtk::ResponseType::RESPONSE_OK);
		add_button("Cancel", Gtk::ResponseType::RESPONSE_CANCEL);
		set_default_response(Gtk::ResponseType::RESPONSE_OK);
		set_default_size(600, 300);

		button_ok->signal_clicked().connect(sigc::mem_fun(this, &PoolBrowserDialogSymbol::ok_clicked));

		store = Gtk::ListStore::create(list_columns);

		SQLite::Query q(pool->db, "SELECT symbols.uuid, symbols.name, units.name FROM symbols,units WHERE symbols.unit = units.uuid AND units.uuid=? ORDER BY symbols.name");
		q.bind(1, unit_uuid);
		Gtk::TreeModel::Row row;
		unsigned int n = 0;
		while(q.step()) {
			row = *(store->append());
			row[list_columns.uuid] = q.get<std::string>(0);
			row[list_columns.symbol_name] = q.get<std::string>(1);
			row[list_columns.unit_name] = q.get<std::string>(2);
			n++;
		}
		if(n == 1) {
			selection_valid = true;
			selected_uuid = row[list_columns.uuid];
			Glib::signal_idle().connect( sigc::mem_fun(this, &PoolBrowserDialogSymbol::auto_ok));
			return;
		}


		view = Gtk::manage(new Gtk::TreeView(store));
		view->append_column("Symbol Name", list_columns.symbol_name);
		view->append_column("Unit Name", list_columns.unit_name);
		view->get_selection()->set_mode(Gtk::SelectionMode::SELECTION_BROWSE);
		view->signal_row_activated().connect(sigc::mem_fun(this, &PoolBrowserDialogSymbol::row_activated));
		view->get_selection()->signal_changed().connect([this]{
			auto it = view->get_selection()->get_selected();
			if(it) {
				Gtk::TreeModel::Row r = *it;
				Symbol sym = *pool->get_symbol(r[list_columns.uuid]);
				sym.expand();
				canvas->update(sym);
				auto bbox = sym.get_bbox();
				canvas->zoom_to_bbox(bbox.first, bbox.second);
			}
		});


		auto sc = Gtk::manage(new Gtk::ScrolledWindow());
		sc->set_policy(Gtk::POLICY_NEVER, Gtk::POLICY_AUTOMATIC);
		sc->add(*view);
		auto box = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL, 0));
		box->pack_start(*sc, false, false, 0);
		box->pack_start(*Gtk::manage(new Gtk::Separator(Gtk::ORIENTATION_VERTICAL)), false, false, 0);
		canvas = Gtk::manage(new CanvasGL());
		canvas->set_core(core);
		canvas->set_selection_allowed(false);
		box->pack_start(*canvas, true, true, 0);



		get_content_area()->pack_start(*box, true, true, 0);
		get_content_area()->set_border_width(0);

		show_all();
	}
}
