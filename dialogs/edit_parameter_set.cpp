#include "edit_parameter_set.hpp"
#include "widgets/parameter_set_editor.hpp"
#include <iostream>
#include <deque>
#include <algorithm>

namespace horizon {
	ParameterSetDialog::ParameterSetDialog(Gtk::Window *parent, ParameterSet *pset) :
		Gtk::Dialog("Edit parameter set", *parent, Gtk::DialogFlags::DIALOG_MODAL|Gtk::DialogFlags::DIALOG_USE_HEADER_BAR)
		{
		set_default_size(400, 300);
		add_button("Cancel", Gtk::ResponseType::RESPONSE_CANCEL);
		add_button("OK", Gtk::ResponseType::RESPONSE_OK);
		signal_response().connect([this](int r){if(r==Gtk::RESPONSE_OK){ok_clicked();}});
		set_default_response(Gtk::ResponseType::RESPONSE_OK);

		auto editor = Gtk::manage(new ParameterSetEditor(pset));

		get_content_area()->pack_start(*editor, true, true, 0);
		get_content_area()->set_border_width(0);


		show_all();
	}
	void ParameterSetDialog::ok_clicked() {

	}
}
