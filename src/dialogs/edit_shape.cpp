#include "edit_shape.hpp"
#include "common/shape.hpp"
#include <iostream>
#include <deque>
#include <algorithm>
#include "widgets/spin_button_dim.hpp"

namespace horizon {
	ShapeDialog::ShapeDialog(Gtk::Window *parent, Shape *sh) :
		Gtk::Dialog("Edit shape", *parent, Gtk::DialogFlags::DIALOG_MODAL|Gtk::DialogFlags::DIALOG_USE_HEADER_BAR),
		shape(sh)
		{
		add_button("Cancel", Gtk::ResponseType::RESPONSE_CANCEL);
		add_button("OK", Gtk::ResponseType::RESPONSE_OK);
		signal_response().connect([this](int r){if(r==Gtk::RESPONSE_OK){ok_clicked();}});
		set_default_response(Gtk::ResponseType::RESPONSE_OK);

		grid = Gtk::manage(new Gtk::Grid());
		grid->set_column_spacing(10);
		grid->set_row_spacing(10);
		grid->set_margin_bottom(20);
		grid->set_margin_top(20);
		grid->set_margin_end(20);
		grid->set_margin_start(20);

		int top = 0;
		{
			auto la = Gtk::manage(new Gtk::Label("Form"));
			la->set_halign(Gtk::ALIGN_END);
			la->get_style_context()->add_class("dim-label");
			w_form = Gtk::manage(new Gtk::ComboBoxText());
			w_form->append(std::to_string(static_cast<int>(Shape::Form::CIRCLE)), "Circle");
			w_form->append(std::to_string(static_cast<int>(Shape::Form::RECTANGLE)), "Rectangle");
			w_form->append(std::to_string(static_cast<int>(Shape::Form::OBROUND)), "Obround");
			w_form->signal_changed().connect(sigc::mem_fun(this, &ShapeDialog::update));
			w_form->set_active_id(std::to_string(static_cast<int>(shape->form)));
			w_form->set_hexpand(true);

			grid->attach(*la, 0, top, 1, 1);
			grid->attach(*w_form, 1, top, 1, 1);
			top++;
		}
		get_content_area()->pack_start(*grid, true, true, 0);


		show_all();
	}

	SpinButtonDim *ShapeDialog::add_dim(const std::string &text, int top) {
		auto la = Gtk::manage(new Gtk::Label(text));
		la->set_halign(Gtk::ALIGN_END);
		la->get_style_context()->add_class("dim-label");
		auto adj = Gtk::manage(new SpinButtonDim());
		grid->attach(*la, 0, top, 1, 1);
		grid->attach(*adj, 1, top, 1, 1);
		widgets.push_back({la, adj});
		return adj;
	}

	static int64_t get_or_0(const std::vector<int64_t> &v, unsigned int i) {
		if(i<v.size())
			return v[i];
		else
			return 0;
	}

	void ShapeDialog::update() {
		auto form = static_cast<Shape::Form>(std::stoi(w_form->get_active_id()));
		for(auto &it: widgets) {
			grid->remove(*it.first);
			grid->remove(*it.second);
		}
		widgets.clear();
		int top = 1;
		if(form == Shape::Form::RECTANGLE || form == Shape::Form::OBROUND) {
			auto w = add_dim("Width", top++);
			w->set_range(0, 1000_mm);
			w->set_value(get_or_0(shape->params, 0));

			auto w2 = add_dim("Height", top++);
			w2->set_range(0, 1000_mm);
			w2->set_value(get_or_0(shape->params, 1));

			w->signal_activate().connect([w2] {w2->grab_focus();});
			w2->signal_activate().connect([this] {response(Gtk::RESPONSE_OK);});
			w->grab_focus();
		}
		else if(form == Shape::Form::CIRCLE) {
			auto w = add_dim("Diameter", top++);
			w->set_range(0, 1000_mm);
			w->set_value(get_or_0(shape->params, 0));
			w->signal_activate().connect([this] {response(Gtk::RESPONSE_OK);});
			w->grab_focus();
		}
		grid->show_all();
	}

	void ShapeDialog::ok_clicked() {
		shape->form = static_cast<Shape::Form>(std::stoi(w_form->get_active_id()));
		shape->params.clear();
		for(auto &it: widgets) {
			auto sp = dynamic_cast<SpinButtonDim*>(it.second);
			shape->params.push_back(sp->get_value_as_int());
		}

	}
}
