#include "ask_datum_string.hpp"
#include <iostream>

namespace horizon {


	AskDatumStringDialog::AskDatumStringDialog(Gtk::Window *parent, const std::string &label) :
		Gtk::Dialog("Enter Datum", *parent, Gtk::DialogFlags::DIALOG_MODAL|Gtk::DialogFlags::DIALOG_USE_HEADER_BAR)
		{
		add_button("Cancel", Gtk::ResponseType::RESPONSE_CANCEL);
		add_button("OK", Gtk::ResponseType::RESPONSE_OK);

		set_default_response(Gtk::ResponseType::RESPONSE_OK);


		auto box = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_VERTICAL, 4));
		auto la = Gtk::manage(new Gtk::Label(label));
		la->set_halign(Gtk::ALIGN_START);
		box->pack_start(*la, false, false, 0);

		entry = Gtk::manage(new Gtk::Entry());
		entry->signal_activate().connect([this]{response(Gtk::RESPONSE_OK);});
		box->pack_start(*entry, false, false, 0);
		get_content_area()->pack_start(*box, true, true, 0);
		show_all();

	}
}
