#include "pool_browser.hpp"
#include "pool.hpp"
#include <set>

namespace horizon {
	PoolBrowser::PoolBrowser(Pool *p) :
		Gtk::Box(Gtk::ORIENTATION_VERTICAL,0),
		pool(p) {
	}

	Gtk::Entry *PoolBrowser::create_search_entry(const std::string &label) {
		auto entry = Gtk::manage(new Gtk::Entry());
		add_search_widget(label, *entry);
		entry->signal_changed().connect(sigc::mem_fun(this, &PoolBrowser::search));
		return entry;
	}

	void PoolBrowser::add_search_widget(const std::string &label, Gtk::Widget &w) {
		auto la = Gtk::manage(new Gtk::Label(label));
		la->get_style_context()->add_class("dim-label");
		la->set_halign(Gtk::ALIGN_END);
		grid->attach(*la, 0, grid_top, 1, 1);
		w.set_hexpand(true);
		grid->attach(w, 1, grid_top, 1, 1);
		grid_top++;
		la->show();
		w.show();
	}

	void PoolBrowser::construct() {

		store = create_list_store();

		grid = Gtk::manage(new Gtk::Grid());
		grid->set_column_spacing(10);
		grid->set_row_spacing(10);

		grid->set_margin_top(20);
		grid->set_margin_start(20);
		grid->set_margin_end(20);
		grid->set_margin_bottom(20);

		grid->show();
		pack_start(*grid, false, false, 0);

		auto sep = Gtk::manage(new Gtk::Separator(Gtk::ORIENTATION_HORIZONTAL));
		sep->show();
		pack_start(*sep, false, false, 0);

		treeview = Gtk::manage(new Gtk::TreeView(store));

		auto sc = Gtk::manage(new Gtk::ScrolledWindow());
		sc->add(*treeview);
		sc->set_policy(Gtk::POLICY_NEVER, Gtk::POLICY_AUTOMATIC);
		sc->show();

		pack_start(*sc, true, true, 0);
		treeview->show();

		create_columns();
		treeview->get_selection()->set_mode(Gtk::SelectionMode::SELECTION_BROWSE);

		sort_controller = std::make_unique<SortController>(treeview);
		sort_controller->set_simple(true);
		add_sort_controller_columns();
		sort_controller->set_sort(0, SortController::Sort::ASC);
		sort_controller->signal_changed().connect(sigc::mem_fun(this, &PoolBrowser::search));

		//dynamic_cast<Gtk::CellRendererText*>(view->get_column_cell_renderer(3))->property_ellipsize() = Pango::ELLIPSIZE_END;
		treeview->get_selection()->set_mode(Gtk::SelectionMode::SELECTION_BROWSE);
		treeview->signal_row_activated().connect(sigc::mem_fun(this, &PoolBrowser::row_activated));
		treeview->get_selection()->signal_changed().connect(sigc::mem_fun(this, &PoolBrowser::selection_changed));
		if(path_column > 0) {
			treeview->get_column(path_column)->set_visible(false);
		}
	}

	void PoolBrowser::row_activated(const Gtk::TreeModel::Path& path, Gtk::TreeViewColumn* column) {
		auto it = store->get_iter(path);
		if(it) {
			s_signal_activated.emit();
		}
	}

	void PoolBrowser::selection_changed() {
		s_signal_selected.emit();
	}

	UUID PoolBrowser::get_selected() {
		auto it = treeview->get_selection()->get_selected();
		if(it) {
			Gtk::TreeModel::Row row = *it;
			return uuid_from_row(row);
		}
		return UUID();
	}

	void PoolBrowser::set_show_none(bool v) {
		show_none = v;
		search();
	}

	void PoolBrowser::set_show_path(bool v) {
		show_path = v;
		if(path_column > 0) {
			if(auto col = treeview->get_column(path_column))
				col->set_visible(v);
		}
		search();
	}

	void PoolBrowser::scroll_to_selection() {
		auto it = treeview->get_selection()->get_selected();
		if(it) {
			auto path = treeview->get_model()->get_path(it);
			treeview->scroll_to_cell(path, *treeview->get_column(0));
		}
	}

	void PoolBrowser::select_uuid(const UUID &uu) {
		for(const auto &it: store->children()) {
			if(uuid_from_row(*it) == uu) {
				treeview->get_selection()->select(it);
				break;
			}
		}
	}
}
