#include "select_net.hpp"
#include <iostream>

namespace horizon {

	void SelectNetDialog::ok_clicked() {
		auto n = net_selector->get_selected_net();
		if(n) {
			valid = true;
			net = n;
			response(Gtk::ResponseType::RESPONSE_OK);
		}

	}

	void SelectNetDialog::net_selected(const UUID &uu) {
		valid = true;
		net = uu;
		response(Gtk::ResponseType::RESPONSE_OK);
	}


	SelectNetDialog::SelectNetDialog(Gtk::Window *parent, Block *b, const std::string &ti) :
		Gtk::Dialog(ti, *parent, Gtk::DialogFlags::DIALOG_MODAL|Gtk::DialogFlags::DIALOG_USE_HEADER_BAR),
		block(b)
		{
		add_button("Cancel", Gtk::ResponseType::RESPONSE_CANCEL);
		auto ok_button = add_button("OK", Gtk::ResponseType::RESPONSE_OK);
		set_default_response(Gtk::ResponseType::RESPONSE_OK);
		set_position(Gtk::WIN_POS_MOUSE);
		set_default_size(300, 300);

		net_selector = Gtk::manage(new NetSelector(b));
		net_selector->signal_activated().connect(sigc::mem_fun(this, &SelectNetDialog::net_selected));
		ok_button->signal_clicked().connect([this]{net = net_selector->get_selected_net(); valid=net!=UUID();});


		get_content_area()->pack_start(*net_selector, true, true, 0);
		get_content_area()->set_border_width(0);

		show_all();
	}
}
