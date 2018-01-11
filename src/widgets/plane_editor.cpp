#include "plane_editor.hpp"
#include "board/plane.hpp"
#include "util/gtk_util.hpp"
#include "spin_button_dim.hpp"

namespace horizon {
	PlaneEditor::PlaneEditor(PlaneSettings *sets, int *priority): Gtk::Grid(), settings(sets) {
		set_column_spacing(10);
		set_row_spacing(10);

		int top = 0;
		if(priority) {
			auto sp = Gtk::manage(new Gtk::SpinButton());
			sp->set_range(0, 100);
			sp->set_increments(1, 1);
			bind_widget(sp, *priority);
			grid_attach_label_and_widget(this, "Priority", sp, top);
		}


		{
			auto sp = Gtk::manage(new SpinButtonDim());
			sp->set_range(0, 10_mm);
			bind_widget(sp, settings->min_width);
			auto la = grid_attach_label_and_widget(this, "Minimum width", sp, top);
			widgets_from_rules_disable.insert(la);
			widgets_from_rules_disable.insert(sp);
		}
		{
			auto sw = Gtk::manage(new Gtk::Switch());
			sw->set_halign(Gtk::ALIGN_START);
			bind_widget(sw, settings->keep_orphans);
			auto la = grid_attach_label_and_widget(this, "Keep orphans", sw, top);
			widgets_from_rules_disable.insert(la);
			widgets_from_rules_disable.insert(sw);
		}
		{
			auto box = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL));
			box->get_style_context()->add_class("linked");
			auto b1 = Gtk::manage(new Gtk::RadioButton("Round"));
			b1->set_mode(false);
			box->pack_start(*b1, true, true, 0);

			auto b2 = Gtk::manage(new Gtk::RadioButton("Square"));
			b2->set_mode(false);
			b2->join_group(*b1);
			box->pack_start(*b2, true, true, 0);

			auto b3 = Gtk::manage(new Gtk::RadioButton("Miter"));
			b3->set_mode(false);
			b3->join_group(*b1);
			box->pack_start(*b3, true, true, 0);

			std::map<PlaneSettings::Style, Gtk::RadioButton*> style_widgets = {
				{PlaneSettings::Style::ROUND, b1},
				{PlaneSettings::Style::SQUARE, b2},
				{PlaneSettings::Style::MITER, b3},
			};

			bind_widget(style_widgets, settings->style);

			auto la = grid_attach_label_and_widget(this, "Style", box, top);
			widgets_from_rules_disable.insert(la);
			widgets_from_rules_disable.insert(box);
		}
		{
			auto box = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL));
			box->get_style_context()->add_class("linked");
			auto b1 = Gtk::manage(new Gtk::RadioButton("Expand"));
			b1->set_mode(false);
			box->pack_start(*b1, true, true, 0);

			auto b2 = Gtk::manage(new Gtk::RadioButton("Bounding box"));
			b2->set_mode(false);
			b2->join_group(*b1);
			box->pack_start(*b2, true, true, 0);

			std::map<PlaneSettings::TextStyle, Gtk::RadioButton*> style_widgets = {
				{PlaneSettings::TextStyle::EXPAND, b1},
				{PlaneSettings::TextStyle::BBOX, b2},
			};

			bind_widget(style_widgets, settings->text_style);

			auto la = grid_attach_label_and_widget(this, "Text style", box, top);
			widgets_from_rules_disable.insert(la);
			widgets_from_rules_disable.insert(box);
		}
		{
			auto box = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL));
			box->get_style_context()->add_class("linked");
			auto b1 = Gtk::manage(new Gtk::RadioButton("Solid"));
			b1->set_mode(false);
			box->pack_start(*b1, true, true, 0);

			auto b2 = Gtk::manage(new Gtk::RadioButton("Thermal relief"));
			b2->set_mode(false);
			b2->join_group(*b1);
			box->pack_start(*b2, true, true, 0);

			std::map<PlaneSettings::ConnectStyle, Gtk::RadioButton*> style_widgets = {
				{PlaneSettings::ConnectStyle::SOLID, b1},
				{PlaneSettings::ConnectStyle::THERMAL, b2},
			};

			bind_widget(style_widgets, settings->connect_style);

			b1->signal_toggled().connect(sigc::mem_fun(this, &PlaneEditor::update_thermal));
			b2->signal_toggled().connect(sigc::mem_fun(this, &PlaneEditor::update_thermal));

			auto la = grid_attach_label_and_widget(this, "Connect style", box, top);
			widgets_from_rules_disable.insert(la);
			widgets_from_rules_disable.insert(box);
		}
		{
			auto sp = Gtk::manage(new SpinButtonDim());
			sp->set_range(0, 10_mm);
			bind_widget(sp, settings->thermal_gap_width);
			auto la = grid_attach_label_and_widget(this, "Th. gap", sp, top);
			widgets_from_rules_disable.insert(la);
			widgets_from_rules_disable.insert(sp);
			widgets_thermal_only.insert(la);
			widgets_thermal_only.insert(sp);
		}
		{
			auto sp = Gtk::manage(new SpinButtonDim());
			sp->set_range(0, 10_mm);
			bind_widget(sp, settings->thermal_spoke_width);
			auto la = grid_attach_label_and_widget(this, "Th. spoke width", sp, top);
			widgets_from_rules_disable.insert(la);
			widgets_from_rules_disable.insert(sp);
			widgets_thermal_only.insert(la);
			widgets_thermal_only.insert(sp);
		}
		for(auto &it: widgets_thermal_only) {
			it->set_no_show_all();
			it->show();
		}
		update_thermal();
	}

	void PlaneEditor::update_thermal() {
		for(auto &it: widgets_thermal_only) {
			it->set_visible(settings->connect_style == PlaneSettings::ConnectStyle::THERMAL);
		}
	}

	void PlaneEditor::set_from_rules(bool v) {
		for(auto &it: widgets_from_rules_disable) {
			it->set_sensitive(!v);
		}
	}
}
