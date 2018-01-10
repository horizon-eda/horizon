#include "ask_datum.hpp"
#include "widgets/spin_button_dim.hpp"
#include <iostream>

namespace horizon {


	AskDatumDialog::AskDatumDialog(Gtk::Window *parent, const std::string &label, bool mode_xy ) :
		Gtk::Dialog("Enter Datum", *parent, Gtk::DialogFlags::DIALOG_MODAL|Gtk::DialogFlags::DIALOG_USE_HEADER_BAR)
		{
		add_button("Cancel", Gtk::ResponseType::RESPONSE_CANCEL);
		add_button("OK", Gtk::ResponseType::RESPONSE_OK);

		set_default_response(Gtk::ResponseType::RESPONSE_OK);


		auto box = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_VERTICAL, 4));
		auto la = Gtk::manage(new Gtk::Label(label));
		la->set_halign(Gtk::ALIGN_START);
		box->pack_start(*la, false, false, 0);

		if(mode_xy == false) {
			sp = Gtk::manage(new SpinButtonDim());
			sp->set_margin_start(8);
			sp->set_range(0, 1e9);
			sp->signal_activate().connect([this]{response(Gtk::RESPONSE_OK);});
			box->pack_start(*sp, false, false, 0);
		}
		else {
			auto xbox = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL, 4));
			cb_x = Gtk::manage(new Gtk::CheckButton("X"));
			cb_x->set_active(true);
			sp_x = sp = Gtk::manage(new SpinButtonDim());
			sp_x->set_range(-1e9, 1e9);
			sp_x->signal_activate().connect([this]{sp_y->grab_focus();});
			xbox->pack_start(*cb_x, false, false, 0);
			xbox->pack_start(*sp_x, true, true, 0);
			box->pack_start(*xbox, false, false, 0);

			auto ybox = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL, 4));
			cb_y = Gtk::manage(new Gtk::CheckButton("Y"));
			cb_y->set_active(true);
			sp_y = sp = Gtk::manage(new SpinButtonDim());
			sp_y->set_range(-1e9, 1e9);
			sp_y->signal_activate().connect([this]{response(Gtk::RESPONSE_OK);});
			ybox->pack_start(*cb_y, false, false, 0);
			ybox->pack_start(*sp_y, true, true, 0);
			box->pack_start(*ybox, false, false, 0);
			sp_x->grab_focus();
		}

		get_content_area()->pack_start(*box, true, true, 0);
		show_all();

	}
}
