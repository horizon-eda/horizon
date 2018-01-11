#include "edit_via.hpp"
#include "widgets/parameter_set_editor.hpp"
#include "widgets/chooser_buttons.hpp"
#include "board/via.hpp"
#include "board/via_padstack_provider.hpp"
#include <iostream>
#include <deque>
#include <algorithm>

namespace horizon {
	EditViaDialog::EditViaDialog(Gtk::Window *parent, Via *via, ViaPadstackProvider &vpp) :
		Gtk::Dialog("Edit via", *parent, Gtk::DialogFlags::DIALOG_MODAL|Gtk::DialogFlags::DIALOG_USE_HEADER_BAR)
		{
		set_default_size(400, 300);
		add_button("Cancel", Gtk::ResponseType::RESPONSE_CANCEL);
		add_button("OK", Gtk::ResponseType::RESPONSE_OK);
		set_default_response(Gtk::ResponseType::RESPONSE_OK);

		auto box = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_VERTICAL, 0));

		auto bbox = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL, 8));
		bbox->set_margin_start(8);
		bbox->set_margin_end(8);
		bbox->set_margin_top(8);

		cb_from_rules = Gtk::manage(new Gtk::CheckButton("From Rules"));
		cb_from_rules->set_active(via->from_rules);
		cb_from_rules->signal_toggled().connect([this, via] {
			via->from_rules = cb_from_rules->get_active();
			update_sensitive();
		});
		bbox->pack_start(*cb_from_rules, true, true, 0);

		button_vp = Gtk::manage(new ViaPadstackButton(vpp));
		button_vp->property_selected_uuid() = via->vpp_padstack->uuid;
		button_vp->property_selected_uuid().signal_changed().connect([this, via, &vpp]{
			if(!via->from_rules)
				via->vpp_padstack = vpp.get_padstack(button_vp->property_selected_uuid());
		});
		bbox->pack_start(*button_vp, true, true, 0);

		box->pack_start(*bbox, false, false, 0);

		editor = Gtk::manage(new ParameterSetEditor(&via->parameter_set));
		box->pack_start(*editor, true, true, 0);

		update_sensitive();

		get_content_area()->pack_start(*box, true, true, 0);
		get_content_area()->set_border_width(0);

		show_all();
	}

	void EditViaDialog::update_sensitive() {
		auto s = !cb_from_rules->get_active();
		button_vp->set_sensitive(s);
		editor->set_sensitive(s);
	}
}
