#include "warnings_box.hpp"
#include <algorithm>
#include <iostream>

namespace horizon {
	WarningsBox::WarningsBox():
		Gtk::Box(Gtk::Orientation::ORIENTATION_VERTICAL, 4)
	{
		auto *la = Gtk::manage(new Gtk::Label());
		la->set_markup("<b>Warnings</b>");
		la->show();
		pack_start(*la, false, false, 0);

		store = Gtk::ListStore::create(list_columns);
		view = Gtk::manage(new Gtk::TreeView(store));
		view->append_column("Text", list_columns.text);
		view->show();
		auto sc = Gtk::manage(new Gtk::ScrolledWindow());
		sc->set_shadow_type(Gtk::SHADOW_IN);
		sc->set_min_content_height(100);
		sc->set_policy(Gtk::POLICY_NEVER, Gtk::POLICY_AUTOMATIC);
		sc->add(*view);
		sc->show_all();

		view->signal_row_activated().connect(sigc::mem_fun(this, &WarningsBox::row_activated));
		pack_start(*sc, true, true, 0);
		set_visible(false);
	}

	void WarningsBox::row_activated(const Gtk::TreeModel::Path& path, Gtk::TreeViewColumn* column) {
			auto it = store->get_iter(path);
			if(it) {
				Gtk::TreeModel::Row row = *it;
				s_signal_selected.emit(row[list_columns.position]);



			}
		}

	void WarningsBox::update(const std::vector<Warning> &warnings) {

		Gtk::TreeModel::Row row;
		store->freeze_notify();
		store->clear();
		set_visible(warnings.size()>0);
		for(const auto &it: warnings) {
			row = *(store->append());
			row[list_columns.text] = it.text;
			row[list_columns.position] = it.position;
		}
		store->thaw_notify();
	}


}
