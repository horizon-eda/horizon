#include "plane_editor.hpp"
#include "plane.hpp"
#include "util/gtk_util.hpp"
#include "spin_button_dim.hpp"

namespace horizon {
	PlaneEditor::PlaneEditor(PlaneSettings *settings, int *priority): Gtk::Grid() {
		set_column_spacing(10);
		set_row_spacing(10);

		int top = 0;
		if(priority) {
			auto la = Gtk::manage(new Gtk::Label("Priority"));
			la->set_halign(Gtk::ALIGN_END);
			la->get_style_context()->add_class("dim-label");
			auto sp = Gtk::manage(new Gtk::SpinButton());
			sp->set_range(0, 100);
			sp->set_increments(1, 1);
			bind_widget(sp, *priority);
			attach(*la, 0, top, 1, 1);
			attach(*sp, 1, top, 1, 1);
			top++;
		}


		{
			auto la = Gtk::manage(new Gtk::Label("Minimum width"));
			la->set_halign(Gtk::ALIGN_END);
			la->get_style_context()->add_class("dim-label");
			auto sp = Gtk::manage(new SpinButtonDim());
			sp->set_range(0, 10_mm);
			bind_widget(sp, settings->min_width);
			attach(*la, 0, top, 1, 1);
			attach(*sp, 1, top, 1, 1);
			widgets_from_rules_disable.insert(la);
			widgets_from_rules_disable.insert(sp);
			top++;
		}
		{
			auto la = Gtk::manage(new Gtk::Label("Keep orphans"));
			la->set_halign(Gtk::ALIGN_END);
			la->get_style_context()->add_class("dim-label");
			auto sw = Gtk::manage(new Gtk::Switch());
			sw->set_halign(Gtk::ALIGN_START);
			bind_widget(sw, settings->keep_orphans);
			attach(*la, 0, top, 1, 1);
			attach(*sw, 1, top, 1, 1);
			widgets_from_rules_disable.insert(la);
			widgets_from_rules_disable.insert(sw);
			top++;
		}
		{
			auto la = Gtk::manage(new Gtk::Label("Style"));
			la->set_halign(Gtk::ALIGN_END);
			la->get_style_context()->add_class("dim-label");
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

			b1->set_active(settings->style==PlaneSettings::Style::ROUND);
			b2->set_active(settings->style==PlaneSettings::Style::SQUARE);
			b3->set_active(settings->style==PlaneSettings::Style::MITER);

			b1->signal_toggled().connect([b1, settings]{
				if(b1->get_active())
					settings->style = PlaneSettings::Style::ROUND;
			});
			b2->signal_toggled().connect([b2, settings]{
				if(b2->get_active())
					settings->style = PlaneSettings::Style::SQUARE;
			});
			b3->signal_toggled().connect([b3, settings]{
				if(b3->get_active())
					settings->style = PlaneSettings::Style::MITER;
			});

			attach(*la, 0, top, 1, 1);
			attach(*box, 1, top, 1, 1);
			widgets_from_rules_disable.insert(la);
			widgets_from_rules_disable.insert(box);
			top++;
		}
	}

	void PlaneEditor::set_from_rules(bool v) {
		for(auto &it: widgets_from_rules_disable) {
			it->set_sensitive(!v);
		}
	}
}
