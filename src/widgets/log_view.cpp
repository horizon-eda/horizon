#include "log_view.hpp"
#include <iostream>

namespace horizon {
	LogView::LogView(): Gtk::Box(Gtk::ORIENTATION_VERTICAL, 0) {
		bbox = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL, 8));
		bbox->set_margin_bottom(8);
		bbox->set_margin_top(8);
		bbox->set_margin_start(8);
		bbox->set_margin_end(8);

		auto clear_button = Gtk::manage(new Gtk::Button("Clear"));
		clear_button->signal_clicked().connect([this] {
			store->clear();
		});
		bbox->pack_start(*clear_button, false, false, 0);

		{
			auto lbox = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL, 0));
			lbox->get_style_context()->add_class("linked");

			auto add_level_button = [this, lbox] (Logger::Level l) {
				auto bu = Gtk::manage(new Gtk::ToggleButton(Logger::level_to_string(l)));
				bu->signal_toggled().connect([this, bu, l] {
					if(bu->get_active()) {
						levels_visible.insert(l);
					}
					else {
						if(levels_visible.count(l))
							levels_visible.erase(l);
					}
					if(store_filtered)
						store_filtered->refilter();
				});

				lbox->pack_start(*bu, false, false, 0);
				return bu;
			};

			add_level_button(Logger::Level::CRITICAL)->set_active(true);
			add_level_button(Logger::Level::WARNING)->set_active(true);
			add_level_button(Logger::Level::INFO);
			add_level_button(Logger::Level::DEBUG);

			levels_visible = {Logger::Level::CRITICAL, Logger::Level::WARNING};

			bbox->pack_start(*lbox, false, false, 0);
		}

		auto follow_cb = Gtk::manage(new Gtk::CheckButton("Follow"));
		follow_cb->set_active(true);
		follow_cb->signal_clicked().connect([this, follow_cb] {
			follow = follow_cb->get_active();
			if(follow) {
				auto it = store->children().end();
				it--;
				tree_view->scroll_to_row(store->get_path(it));
			}
		});
		bbox->pack_start(*follow_cb, false, false, 0);


		bbox->show_all();
		pack_start(*bbox,false, false, 0);

		sc = Gtk::manage(new Gtk::ScrolledWindow);
		sc->set_policy(Gtk::POLICY_NEVER, Gtk::POLICY_AUTOMATIC);

		store = Gtk::ListStore::create(list_columns);
		store_filtered = Gtk::TreeModelFilter::create(store);
		store_filtered->set_visible_func([this](const Gtk::TreeModel::const_iterator& it)->bool{
			const Gtk::TreeModel::Row row = *it;
			auto level = row[list_columns.level];
			return levels_visible.count(level);
		});


		tree_view = Gtk::manage(new Gtk::TreeView(store_filtered));
		tree_view->append_column("Seq", list_columns.seq);

		{
			auto cr = Gtk::manage(new Gtk::CellRendererText());
			auto tvc = Gtk::manage(new Gtk::TreeViewColumn("Level", *cr));
			tvc->set_cell_data_func(*cr, [this](Gtk::CellRenderer* tcr, const Gtk::TreeModel::iterator &it){
				Gtk::TreeModel::Row row = *it;
				auto mcr = dynamic_cast<Gtk::CellRendererText*>(tcr);
				mcr->property_text() = Logger::level_to_string(row[list_columns.level]);
			});
			tree_view->append_column(*tvc);
		}
		{
			auto cr = Gtk::manage(new Gtk::CellRendererText());
			auto tvc = Gtk::manage(new Gtk::TreeViewColumn("Domain", *cr));
			tvc->set_cell_data_func(*cr, [this](Gtk::CellRenderer* tcr, const Gtk::TreeModel::iterator &it){
				Gtk::TreeModel::Row row = *it;
				auto mcr = dynamic_cast<Gtk::CellRendererText*>(tcr);
				mcr->property_text() = Logger::domain_to_string(row[list_columns.domain]);
			});
			tree_view->append_column(*tvc);
		}
		tree_view->append_column("Message", list_columns.message);
		tree_view->append_column("Detail", list_columns.detail);

		sc->add(*tree_view);
		pack_start(*sc, true, true, 0);
		sc->show_all();
	}

	void LogView::push_log(const Logger::Item &it) {
		Gtk::TreeModel::Row row = *store->append();
		row[list_columns.seq] = it.seq;
		row[list_columns.domain] = it.domain;
		row[list_columns.message] = it.message;
		row[list_columns.detail] = it.detail;
		row[list_columns.level] = it.level;

		if(follow && tree_view->get_realized()) {
			tree_view->scroll_to_row(store->get_path(row));
		}
		s_signal_logged.emit(it);
	}
}
