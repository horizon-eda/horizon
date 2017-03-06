#include "pool_browser_part.hpp"
#include <iostream>
#include <sqlite3.h>
#include "widgets/pool_browser_part.hpp"

namespace horizon {

	void PoolBrowserDialogPart::set_MPN(const std::string &MPN) {
		browser->set_MPN(MPN);
	}

	void PoolBrowserDialogPart::set_show_none(bool v) {
		browser->set_show_none(v);
	}

	PoolBrowserDialogPart::PoolBrowserDialogPart(Gtk::Window *parent, Pool *pool, const UUID &entity_uuid) :
		Gtk::Dialog("Select Part", *parent, Gtk::DialogFlags::DIALOG_MODAL|Gtk::DialogFlags::DIALOG_USE_HEADER_BAR)
		{

		Gtk::Button *button_ok = add_button("OK", Gtk::ResponseType::RESPONSE_OK);
		add_button("Cancel", Gtk::ResponseType::RESPONSE_CANCEL);
		set_default_response(Gtk::ResponseType::RESPONSE_OK);
		set_default_size(800, 480);

		button_ok->signal_clicked().connect(sigc::mem_fun(this, &PoolBrowserDialogPart::ok_clicked));


		browser = PoolBrowserPart::create(pool,entity_uuid);
		browser->signal_part_activated().connect([this]{
			ok_clicked();
			response(Gtk::RESPONSE_OK);
		});
		get_content_area()->pack_start(*browser, true, true, 0);
		get_content_area()->set_border_width(0);
		browser->unreference();
		show_all();
	}

	void PoolBrowserDialogPart::ok_clicked() {
		auto uu = browser->get_selected_part();
		if(uu) {
			selection_valid = true;
			selected_uuid = uu;
		}
	}
}
