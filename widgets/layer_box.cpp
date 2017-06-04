#include "layer_box.hpp"
#include "cell_renderer_layer_display.hpp"
#include <algorithm>
#include <iostream>

namespace horizon {
	LayerBox::LayerBox(Core *c):
		Glib::ObjectBase(typeid(LayerBox)),
		Gtk::Box(Gtk::Orientation::ORIENTATION_VERTICAL, 2),
		core(c),
		p_property_work_layer(*this, "work-layer"),
		p_property_select_work_layer_only(*this, "select-work-layer-only"),
		p_property_layer_opacity(*this, "layer-opacity")
	{
		auto *la = Gtk::manage(new Gtk::Label());
		la->set_markup("<b>Layers</b>");
		la->show();
		pack_start(*la, false, false, 0);

		store = Gtk::ListStore::create(list_columns);
		store->set_sort_column(list_columns.index, Gtk::SORT_DESCENDING);

		auto fr = Gtk::manage(new Gtk::Frame());
		fr->set_shadow_type(Gtk::SHADOW_IN);
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

		view->show();
		auto sc = Gtk::manage(new Gtk::ScrolledWindow());
		sc->set_policy(Gtk::POLICY_NEVER, Gtk::POLICY_AUTOMATIC);
		//sc->set_shadow_type(Gtk::SHADOW_IN);
		sc->set_min_content_height(300);
		sc->add(*view);
		sc->show_all();
		frb->pack_start(*sc, true, true, 0);

		auto ab = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_VERTICAL, 2));
		ab->set_homogeneous(true);
		ab->set_margin_top(4);
		ab->set_margin_bottom(4);
		ab->set_margin_start(4);
		ab->set_margin_end(4);

		auto sel_only_work_layer_tb = Gtk::manage(new Gtk::CheckButton("Select only on work layer"));
		binding_select_work_layer_only = Glib::Binding::bind_property(sel_only_work_layer_tb->property_active(), property_select_work_layer_only(), Glib::BINDING_BIDIRECTIONAL);
		property_select_work_layer_only() = false;
		ab->pack_start(*sel_only_work_layer_tb);

		auto layer_opacity_box = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL, 4));
		layer_opacity_box->set_margin_start(4);
		auto la2 = Gtk::manage(new Gtk::Label("Layer Opacity"));
		layer_opacity_box->pack_start(*la2, false, false, 0);

		auto adj = Gtk::Adjustment::create(90.0, 10.0, 100.0, 1.0, 10.0, 0.0);
		binding_layer_opacity = Glib::Binding::bind_property(adj->property_value(), property_layer_opacity(), Glib::BINDING_BIDIRECTIONAL);

		auto layer_opacity_scale = Gtk::manage(new Gtk::Scale(adj, Gtk::ORIENTATION_HORIZONTAL));
		layer_opacity_scale->set_digits(0);
		layer_opacity_scale->set_value_pos(Gtk::POS_LEFT);
		layer_opacity_box->pack_start(*layer_opacity_scale, true, true, 0);


		ab->pack_start(*layer_opacity_box);

		frb->show_all();
		frb->pack_start(*ab, false, false, 0);
		pack_start(*fr, true, true, 0);



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
		LayerDisplay::Mode dm;
		switch(row[list_columns.display_mode]) {
			case 0: dm=LayerDisplay::Mode::FILL; break;
			case 1: dm=LayerDisplay::Mode::HATCH; break;
			default: dm=LayerDisplay::Mode::OUTLINE;
		}
		s_signal_set_layer_display.emit(row[list_columns.index], LayerDisplay(row[list_columns.visible], dm, c));
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
			row[list_columns.display_mode] = (row[list_columns.display_mode]+1)%3;
			emit_layer_display(row);
		}
	}



	void LayerBox::update() {
		auto layers = core->get_layer_provider()->get_layers();
		Gtk::TreeModel::Row row;
		store->freeze_notify();
		store->clear();
		for(const auto &it: layers) {
			row = *(store->append());

			row[list_columns.name] = it.second.name;
			row[list_columns.index] = it.first;
			row[list_columns.is_work] = (it.first==p_property_work_layer);
			row[list_columns.visible] = true;
			const auto & co = it.second.color;
			auto c = Gdk::RGBA();
			c.set_rgba(co.r, co.g, co.b);
			row[list_columns.color] = c;
			row[list_columns.display_mode] = 0;
		}
		store->thaw_notify();
	}
	void LayerBox::update_work_layer() {
		auto layers = core->get_layer_provider()->get_layers();
		Gtk::TreeModel::Row row;
		store->freeze_notify();
		for(auto &it: store->children()) {
			it[list_columns.is_work] = (it[list_columns.index]==p_property_work_layer);
		}
		store->thaw_notify();
	}

	json LayerBox::serialize() {
		json j;
		j["layer_opacity"] = property_layer_opacity().get_value();
		for(auto &it: store->children()) {
			json k;
			k["visible"] = static_cast<bool>(it[list_columns.visible]);
			k["display_mode"] = static_cast<int>(it[list_columns.display_mode]);
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
					it[list_columns.display_mode] = static_cast<int>(k["display_mode"]);
					emit_layer_display(it);
				}
			}
		}
	}

}
