#include "edit_plane.hpp"
#include "widgets/net_button.hpp"
#include "widgets/spin_button_dim.hpp"
#include "widgets/plane_editor.hpp"
#include <iostream>
#include <deque>
#include <algorithm>
#include "board/plane.hpp"
#include "block/block.hpp"
#include "board/board.hpp"
#include "util/gtk_util.hpp"

namespace horizon {

	void bind(Gtk::Switch *sw, bool &v) {
		sw->set_active(v);
		sw->property_active().signal_changed().connect([sw, &v]{
				v= sw->get_active();
		});
	}

	EditPlaneDialog::EditPlaneDialog(Gtk::Window *parent, Plane *p, Board *brd, Block *block) :
		Gtk::Dialog("Edit Plane", *parent, Gtk::DialogFlags::DIALOG_MODAL|Gtk::DialogFlags::DIALOG_USE_HEADER_BAR),
		plane(p)
		{
		add_button("Cancel", Gtk::ResponseType::RESPONSE_CANCEL);
		auto ok_button = add_button("OK", Gtk::ResponseType::RESPONSE_OK);
		set_default_response(Gtk::ResponseType::RESPONSE_OK);
		//set_default_size(400, 300);


		auto box = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_VERTICAL, 20));
		box->set_margin_start(20);
		box->set_margin_end(20);
		box->set_margin_top(20);
		box->set_margin_bottom(20);
		box->set_halign(Gtk::ALIGN_CENTER);
		box->set_valign(Gtk::ALIGN_CENTER);

		auto box2 = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL, 20));
		auto from_rules = Gtk::manage(new Gtk::CheckButton("From rules"));
		bind_widget(from_rules, plane->from_rules);
		box2->pack_start(*from_rules, false, false, 0);

		auto net_button = Gtk::manage(new NetButton(block));
		if(plane->net) {
			net_button->set_net(plane->net->uuid);
		}
		else {
			ok_button->set_sensitive(false);
		}

		net_button->signal_changed().connect([this, block, ok_button]  (const UUID &uu){
			plane->net = block->get_net(uu);
			ok_button->set_sensitive(true);
		});
		box2->pack_start(*net_button, true, true, 0);

		box->pack_start(*box2, false, false, 0);

		auto ed = Gtk::manage(new PlaneEditor(&plane->settings, &plane->priority));
		ed->set_from_rules(from_rules->get_active());
		from_rules->signal_toggled().connect([from_rules, ed] {
			ed->set_from_rules(from_rules->get_active());
		});
		box->pack_start(*ed, true, true, 0);

		if(plane->net) {
			auto delete_button = Gtk::manage(new Gtk::Button("Delete Plane"));
			delete_button->signal_clicked().connect([this, brd]{
				brd->planes.erase(plane->uuid);
				response(Gtk::RESPONSE_OK);
			});
			box->pack_start(*delete_button, false, false, 0);
		}


		get_content_area()->pack_start(*box, true, true, 0);

		show_all();
	}
}
