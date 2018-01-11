#include "net_selector.hpp"
#include <algorithm>
#include <iostream>
#include "block/block.hpp"

namespace horizon {
	NetSelector::NetSelector(Block *b):
		Gtk::Box(Gtk::Orientation::ORIENTATION_VERTICAL, 16),
		block(b)
	{

		store = Gtk::ListStore::create(list_columns);


		view = Gtk::manage(new Gtk::TreeView(store));
		view->get_selection()->set_mode(Gtk::SELECTION_BROWSE);
		view->append_column("fixme", list_columns.name);
		view->get_column(0)->set_sort_column(list_columns.name);
		store->set_sort_column(list_columns.name, Gtk::SORT_ASCENDING);
		update();

		view->signal_row_activated().connect(sigc::mem_fun(this, &NetSelector::row_activated));

		auto sc = Gtk::manage(new Gtk::ScrolledWindow());
		sc->add(*view);
		sc->show_all();

		pack_start(*sc, true, true, 0);
	}

	void NetSelector::update() {

		Gtk::TreeModel::Row row;
		store->freeze_notify();
		store->clear();

		if(bus_mode) {
			view->get_column(0)->set_title("Bus");
			for(const auto &it: block->buses) {
				row = *(store->append());
				row[list_columns.name] = it.second.name;
				row[list_columns.uuid] = it.second.uuid;
			}
		}
		else if(bus_member_mode) {
			view->get_column(0)->set_title("Bus Member");
			for(const auto &it: bus->members) {
				row = *(store->append());
				row[list_columns.name] = it.second.name;
				row[list_columns.uuid] = it.second.uuid;
			}
		}
		else {
			view->get_column(0)->set_title("Net");
			for(const auto &it: block->nets) {
				if(it.second.is_named() && (!power_only || it.second.is_power)) {
					row = *(store->append());
					row[list_columns.name] = it.second.name;
					row[list_columns.uuid] = it.second.uuid;
				}
			}
		}
		store->thaw_notify();
		view->grab_focus();
	}

	void NetSelector::set_power_only(bool p) {
		power_only = p;
		update();
	}
	void NetSelector::set_bus_mode(bool b) {
		bus_mode = b;
		update();
	}
	void NetSelector::set_bus_member_mode(const UUID &bus_uuid) {
		bus_member_mode = true;
		bus = &block->buses.at(bus_uuid);
		update();
	}

	UUID NetSelector::get_selected_net() {
		auto it = view->get_selection()->get_selected();
		if(it) {
			Gtk::TreeModel::Row row = *it;
			return row[list_columns.uuid];
		}
		return UUID();
	}

	void NetSelector::row_activated(const Gtk::TreeModel::Path& path, Gtk::TreeViewColumn* column) {
		auto it = store->get_iter(path);
		if(it) {
			Gtk::TreeModel::Row row = *it;
			s_signal_activated.emit(row[list_columns.uuid]);
		}
	}

	void NetSelector::select_net(const UUID &uu) {
		for(const auto &it: store->children()) {
			Gtk::TreeModel::Row row = *it;
			if(row[list_columns.uuid] == uu) {
				view->get_selection()->select(it);
				break;
			}
		}
	}


}
