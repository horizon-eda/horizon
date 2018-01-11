#include "layer_box.hpp"
#include "cell_renderer_layer_display.hpp"
#include "common/layer_provider.hpp"
#include "common/lut.hpp"
#include "util/gtk_util.hpp"
#include <algorithm>
#include <iostream>

namespace horizon {
	LayerBox::LayerBox(LayerProvider *lpr, bool show_title):
		Glib::ObjectBase(typeid(LayerBox)),
		Gtk::Box(Gtk::Orientation::ORIENTATION_VERTICAL, 2),
		lp(lpr),
		p_property_work_layer(*this, "work-layer"),
		p_property_select_work_layer_only(*this, "select-work-layer-only"),
		p_property_layer_opacity(*this, "layer-opacity"),
		p_property_highlight_mode(*this, "highlight-mode")
	{
		if(show_title) {
			auto *la = Gtk::manage(new Gtk::Label());
			la->set_markup("<b>Layers</b>");
			la->show();
			pack_start(*la, false, false, 0);
		}

		store = Gtk::ListStore::create(list_columns);
		store->set_sort_column(list_columns.index, Gtk::SORT_DESCENDING);

		auto fr = Gtk::manage(new Gtk::Frame());
		if(show_title)
			fr->set_shadow_type(Gtk::SHADOW_IN);
		else
			fr->set_shadow_type(Gtk::SHADOW_NONE);
		auto frb = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_VERTICAL, 0));
		fr->add(*frb);

		view = Gtk::manage(new Gtk::TreeView(store));
		view->get_selection()->set_mode(Gtk::SELECTION_NONE);
		property_work_layer().signal_changed().connect(sigc::mem_fun(this, &LayerBox::update_work_layer));
		p_property_work_layer.set_value(0);


		{
			auto cr = Gtk::manage(new Gtk::CellRendererToggle());
			cr->signal_toggled().connect(sigc::mem_fun(this, &LayerBox::work_toggled));
			cr->set_radio(true);
			auto tvc = Gtk::manage(new Gtk::TreeViewColumn("W", *cr));
			tvc->add_attribute(cr->property_active(), list_columns.is_work);
			cr->property_xalign().set_value(1);
			view->append_column(*tvc);
		}
		{
			auto cr = Gtk::manage(new Gtk::CellRendererToggle());
			cr->signal_toggled().connect(sigc::mem_fun(this, &LayerBox::toggled));
			auto tvc = Gtk::manage(new Gtk::TreeViewColumn("V", *cr));
			//tvc->add_attribute(cr->property_active(), list_columns.visible);
			tvc->set_cell_data_func(*cr, sigc::mem_fun(this, &LayerBox::visible_cell_data_func));
			cr->property_xalign().set_value(1);
			view->append_column(*tvc);
		}
		{
			auto cr = Gtk::manage(new CellRendererLayerDisplay());
			cr->signal_activate().connect(sigc::mem_fun(this, &LayerBox::activated));
			auto tvc = Gtk::manage(new Gtk::TreeViewColumn("D", *cr));
			tvc->add_attribute(cr->property_color(), list_columns.color);
			tvc->add_attribute(cr->property_display_mode(), list_columns.display_mode);
			//tvc->add_attribute(*cr, "active", list_columns.visible);
			//cr->property_xalign().set_value(1);
			view->append_column(*tvc);
		}
		{
			auto cr = Gtk::manage(new Gtk::CellRendererText());
			auto tvc = Gtk::manage(new Gtk::TreeViewColumn("I", *cr));
			tvc->add_attribute(cr->property_text(), list_columns.index);
			cr->property_xalign().set_value(1);
			view->append_column(*tvc);
		}
		//view->append_column("", list_columns.index);
		view->append_column("Name", list_columns.name);
		view->signal_button_press_event().connect_notify(sigc::mem_fun(this, &LayerBox::handle_button));

		view->show();
		auto sc = Gtk::manage(new Gtk::ScrolledWindow());
		sc->set_policy(Gtk::POLICY_NEVER, Gtk::POLICY_AUTOMATIC);
		//sc->set_shadow_type(Gtk::SHADOW_IN);
		sc->set_min_content_height(400);
		sc->add(*view);
		sc->show_all();
		frb->pack_start(*sc, true, true, 0);

		auto ab = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_VERTICAL, 2));
		ab->set_margin_top(4);
		ab->set_margin_bottom(4);
		ab->set_margin_start(4);
		ab->set_margin_end(4);

		auto sel_only_work_layer_tb = Gtk::manage(new Gtk::CheckButton("Select only on work layer"));
		binding_select_work_layer_only = Glib::Binding::bind_property(sel_only_work_layer_tb->property_active(), property_select_work_layer_only(), Glib::BINDING_BIDIRECTIONAL);
		property_select_work_layer_only() = false;
		ab->pack_start(*sel_only_work_layer_tb);

		auto layer_opacity_grid = Gtk::manage(new Gtk::Grid);
		layer_opacity_grid->set_hexpand(false);
		layer_opacity_grid->set_hexpand_set(true);
		layer_opacity_grid->set_row_spacing(4);
		layer_opacity_grid->set_column_spacing(4);
		int top = 0;
		layer_opacity_grid->set_margin_start(4);

		auto adj = Gtk::Adjustment::create(90.0, 10.0, 100.0, 1.0, 10.0, 0.0);
		binding_layer_opacity = Glib::Binding::bind_property(adj->property_value(), property_layer_opacity(), Glib::BINDING_BIDIRECTIONAL);

		auto layer_opacity_scale = Gtk::manage(new Gtk::Scale(adj, Gtk::ORIENTATION_HORIZONTAL));
		layer_opacity_scale->set_hexpand(true);
		layer_opacity_scale->set_digits(0);
		layer_opacity_scale->set_value_pos(Gtk::POS_LEFT);
		grid_attach_label_and_widget(layer_opacity_grid, "Layer Opacity", layer_opacity_scale, top);


		auto highlight_mode_combo = Gtk::manage(new Gtk::ComboBoxText);
		highlight_mode_combo->append(std::to_string(static_cast<int>(CanvasGL::HighlightMode::HIGHLIGHT)), "Highlight");
		highlight_mode_combo->append(std::to_string(static_cast<int>(CanvasGL::HighlightMode::DIM)), "Dim other");
		highlight_mode_combo->append(std::to_string(static_cast<int>(CanvasGL::HighlightMode::SHADOW)), "Shadow other");
		highlight_mode_combo->set_hexpand(true);
		highlight_mode_combo->signal_changed().connect([this, highlight_mode_combo]{
			p_property_highlight_mode.set_value(static_cast<CanvasGL::HighlightMode>(std::stoi(highlight_mode_combo->get_active_id())));
		});
		highlight_mode_combo->set_active(1);
		grid_attach_label_and_widget(layer_opacity_grid, "Highlight mode", highlight_mode_combo, top);


		ab->pack_start(*layer_opacity_grid);

		frb->show_all();
		frb->pack_start(*ab, false, false, 0);
		pack_start(*fr, true, true, 0);

		{
			auto item = Gtk::manage(new Gtk::MenuItem("Reset color"));
			item->signal_activate().connect([this] {
				auto layers = lp->get_layers();
				int idx = row_for_menu[list_columns.index];
				const auto & co = layers.at(idx).color;
				auto c = Gdk::RGBA();
				c.set_rgba(co.r, co.g, co.b);
				row_for_menu[list_columns.color] = c;
				emit_layer_display(row_for_menu);
			});
			item->show();
			menu.append(*item);
		}
		{
			auto item = Gtk::manage(new Gtk::MenuItem("Select color"));

			item->signal_activate().connect([this] {
				Gtk::ColorChooserDialog dia("Layer color");
				dia.set_transient_for(*dynamic_cast<Gtk::Window*>(get_ancestor(GTK_TYPE_WINDOW)));
				dia.set_rgba(row_for_menu[list_columns.color]);
				dia.property_show_editor() = true;
				dia.set_use_alpha(false);

				if(dia.run() == Gtk::RESPONSE_OK) {
					row_for_menu[list_columns.color] = dia.get_rgba();
					emit_layer_display(row_for_menu);
				}
			});
			item->show();
			menu.append(*item);
		}



		update();
	}


	void LayerBox::visible_cell_data_func(Gtk::CellRenderer *cr, const Gtk::TreeModel::iterator &it) {
		auto crt = dynamic_cast<Gtk::CellRendererToggle*>(cr);
		Gtk::TreeModel::Row row = *it;
		if(row[list_columns.is_work]) {
			crt->property_active() = true;
			crt->property_sensitive() = false;
		}
		else {
			crt->property_sensitive() = true;
			crt->property_active() = row[list_columns.visible];
		}
	}

	void LayerBox::emit_layer_display(const Gtk::TreeModel::Row &row) {
		const Gdk::RGBA &co = row[list_columns.color];
		Color c(co.get_red(), co.get_green(),co.get_blue());
		s_signal_set_layer_display.emit(row[list_columns.index], LayerDisplay(row[list_columns.visible], row[list_columns.display_mode], c));
	}


	void LayerBox::toggled(const Glib::ustring& path) {
		auto it = store->get_iter(path);
		if(it) {
			Gtk::TreeModel::Row row = *it;
			row[list_columns.visible] = !row[list_columns.visible];
			emit_layer_display(row);
		}
	}
	void LayerBox::work_toggled(const Glib::ustring& path) {
		for(auto &it: store->children()) {
			it[list_columns.is_work] = false;
		}

		auto it = store->get_iter(path);
		if(it) {
			Gtk::TreeModel::Row row = *it;
			row[list_columns.is_work] = true;
			property_work_layer().set_value(row[list_columns.index]);
		}
	}

	void LayerBox::activated(const Glib::ustring& path) {
		auto it = store->get_iter(path);
		if(it) {
			Gtk::TreeModel::Row row = *it;
			LayerDisplay::Mode mode = row[list_columns.display_mode];
			auto new_mode = static_cast<LayerDisplay::Mode>((static_cast<int>(mode)+1)%static_cast<int>(LayerDisplay::Mode::N_MODES));
			row[list_columns.display_mode] = new_mode;
			emit_layer_display(row);
		}
	}



	void LayerBox::update() {
		auto layers = lp->get_layers();
		Gtk::TreeModel::Row row;
		std::set<int> layers_from_lp;
		std::set<int> layers_from_store;
		for(const auto &it: layers) {
			layers_from_lp.emplace(it.first);
		}

		store->freeze_notify();
		auto ch = store->children();
		for(auto it = ch.begin(); it!=ch.end();) {
			row = *it;
			if(layers_from_lp.count(row[list_columns.index])==0) {
				store->erase(it++);
			}
			else {
				layers_from_store.insert(row[list_columns.index]);
				it++;
			}
		}

		for(const auto &it: layers) {
			if(layers_from_store.count(it.first) == 0) {
				row = *(store->append());

				row[list_columns.name] = it.second.name;
				row[list_columns.index] = it.first;
				row[list_columns.is_work] = (it.first==p_property_work_layer);
				row[list_columns.visible] = true;
				const auto & co = it.second.color;
				auto c = Gdk::RGBA();
				c.set_rgba(co.r, co.g, co.b);
				row[list_columns.color] = c;
				row[list_columns.display_mode] = LayerDisplay::Mode::FILL;
				emit_layer_display(row);
			}
		}
		store->thaw_notify();
	}

	void LayerBox::handle_button(GdkEventButton* button_event) {
		if(button_event->window == view->get_bin_window()->gobj() && button_event->button == 3) {
			Gtk::TreeModel::Path path;
			if(view->get_path_at_pos(button_event->x, button_event->y, path)) {
				auto it = store->get_iter(path);
				row_for_menu = *it;
				#if GTK_CHECK_VERSION(3,22,0)
					menu.popup_at_pointer((GdkEvent*)button_event);
				#else
					menu.popup(button_event->button, button_event->time);
				#endif
			}
		}
	}

	void LayerBox::update_work_layer() {
		auto layers = lp->get_layers();
		Gtk::TreeModel::Row row;
		store->freeze_notify();
		for(auto &it: store->children()) {
			it[list_columns.is_work] = (it[list_columns.index]==p_property_work_layer);
		}
		store->thaw_notify();
	}

	static const LutEnumStr<LayerDisplay::Mode> dm_lut = {
		{"outline", LayerDisplay::Mode::OUTLINE},
		{"hatch", LayerDisplay::Mode::HATCH},
		{"fill", LayerDisplay::Mode::FILL},
		{"fill_only", LayerDisplay::Mode::FILL_ONLY},
	};

	static json rgba_to_json(const Gdk::RGBA &c) {
		json j;
		j["r"] = c.get_red();
		j["g"] = c.get_green();
		j["b"] = c.get_blue();
		return j;
	}

	static Gdk::RGBA rgba_from_json(const json &j) {
		Gdk::RGBA r;
		r.set_red(j.value("r", 0.0));
		r.set_green(j.value("g", 0.0));
		r.set_blue(j.value("b", 0.0));
		return r;
	}

	json LayerBox::serialize() {
		json j;
		j["layer_opacity"] = property_layer_opacity().get_value();
		for(auto &it: store->children()) {
			json k;
			k["visible"] = static_cast<bool>(it[list_columns.visible]);
			k["display_mode"] = dm_lut.lookup_reverse(it[list_columns.display_mode]);
			k["color"] = rgba_to_json(it[list_columns.color]);
			int index = it[list_columns.index];
			j["layers"][std::to_string(index)] = k;
		}
		return j;
	}

	void LayerBox::load_from_json(const json &j) {
		if(j.count("layers")) {
			property_layer_opacity() = j.value("layer_opacity", 90);
			const auto &j2 = j.at("layers");
			for(auto &it: store->children()) {
				std::string index_str = std::to_string(it[list_columns.index]);
				if(j2.count(index_str)) {
					auto &k = j2.at(index_str);
					it[list_columns.visible] = static_cast<bool>(k["visible"]);
					try {
						it[list_columns.display_mode] = dm_lut.lookup(k["display_mode"]);
					}
					catch(...) {
						it[list_columns.display_mode] = LayerDisplay::Mode::FILL;
					}
					if(k.count("color")){
						it[list_columns.color] = rgba_from_json(k.at("color"));
					}
					emit_layer_display(it);
				}
			}
		}
	}

}
