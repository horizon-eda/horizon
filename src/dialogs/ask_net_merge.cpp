#include "ask_net_merge.hpp"
#include <iostream>

namespace horizon {

	void AskNetMergeDialog::ok_clicked() {

	}


	AskNetMergeDialog::AskNetMergeDialog(Gtk::Window *parent, Net *a, Net *b) :
		Gtk::Dialog("Merge nets", *parent, Gtk::DialogFlags::DIALOG_MODAL|Gtk::DialogFlags::DIALOG_USE_HEADER_BAR),
		net(a),
		into(b){
		add_button("Cancel", Gtk::ResponseType::RESPONSE_CANCEL);
		set_default_response(Gtk::ResponseType::RESPONSE_CANCEL);
		set_position(Gtk::WIN_POS_MOUSE);
		//set_default_size(300, 300);


		auto *box = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_VERTICAL, 10));
		auto *ba = Gtk::manage(new Gtk::Button("Merge \""+net->name+"\" into \""+into->name+"\""));
		box->pack_start(*ba, true, true, 0);
		auto *bb = Gtk::manage(new Gtk::Button("Merge \""+into->name+"\" into \""+net->name+"\""));
		box->pack_start(*bb, true, true, 0);

		ba->signal_clicked().connect(sigc::bind<int>(sigc::mem_fun(this, &Gtk::Dialog::response), 1));
		bb->signal_clicked().connect(sigc::bind<int>(sigc::mem_fun(this, &Gtk::Dialog::response), 2));

		get_content_area()->pack_start(*box, true, true, 0);
		get_content_area()->set_border_width(10);

		show_all();
	}
}
