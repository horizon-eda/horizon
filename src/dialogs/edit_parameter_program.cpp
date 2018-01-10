#include "edit_parameter_program.hpp"
#include "parameter/program.hpp"
#include <iostream>
#include <deque>
#include <algorithm>

namespace horizon {
	ParameterProgramDialog::ParameterProgramDialog(Gtk::Window *parent, ParameterProgram *pgm) :
		Gtk::Dialog("Edit parameter program", *parent, Gtk::DialogFlags::DIALOG_MODAL|Gtk::DialogFlags::DIALOG_USE_HEADER_BAR),
		program(pgm)
		{
		set_default_size(400, 300);
		add_button("Cancel", Gtk::ResponseType::RESPONSE_CANCEL);
		add_button("OK", Gtk::ResponseType::RESPONSE_OK);
		signal_response().connect([this](int r){if(r==Gtk::RESPONSE_OK){ok_clicked();}});
		set_default_response(Gtk::ResponseType::RESPONSE_OK);

		auto box = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_VERTICAL));
		bar = Gtk::manage(new Gtk::InfoBar());
		bar_label = Gtk::manage(new Gtk::Label("fixme"));
		bar_label->set_xalign(0);
		dynamic_cast<Gtk::Box*>(bar->get_content_area())->pack_start(*bar_label, true, true, 0);


		box->pack_start(*bar, false, false, 0);

		auto sc = Gtk::manage(new Gtk::ScrolledWindow());
		tv = Gtk::manage(new Gtk::TextView());
		tv->get_buffer()->set_text(program->get_code());
		tv->set_monospace(true);
		sc->add(*tv);
		box->pack_start(*sc, true, true, 0);

		get_content_area()->pack_start(*box, true, true, 0);
		get_content_area()->set_border_width(0);


		show_all();
		bar->set_visible(false);
	}
	void ParameterProgramDialog::ok_clicked() {
		auto r = program->set_code(tv->get_buffer()->get_text());
		valid = !r.first;
		if(r.first) {
			bar->set_visible(true);
			bar->set_size_request(0,0);
			bar->set_size_request(-1,-1);
			bar_label->set_text(r.second);
		}
	}
}
