#include "ask_delete_component.hpp"
#include <iostream>

namespace horizon {


	AskDeleteComponentDialog::AskDeleteComponentDialog(Gtk::Window *parent, Component *comp) :
		Gtk::Dialog("Delete component", *parent, Gtk::DialogFlags::DIALOG_MODAL|Gtk::DialogFlags::DIALOG_USE_HEADER_BAR)
		{
		add_button("Keep", Gtk::ResponseType::RESPONSE_CANCEL);
		auto *del_button = add_button("Delete", Gtk::ResponseType::RESPONSE_OK);
		del_button->get_style_context()->add_class("destructive-action");
		set_default_response(Gtk::ResponseType::RESPONSE_CANCEL);
		//set_default_size(300, 300);

		auto *la = Gtk::manage(new Gtk::Label("You've unmapped the last gate of "+comp->refdes+".\nDelete Component "+comp->refdes+"?"));
		la->set_margin_top(10);
		la->set_margin_bottom(10);


		get_content_area()->pack_start(*la, true, true, 0);

		show_all();
	}
}
