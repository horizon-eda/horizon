#include "pool_browser_dialog.hpp"
#include "widgets/pool_browser_unit.hpp"
#include "widgets/pool_browser_part.hpp"
#include "widgets/pool_browser_entity.hpp"
#include "widgets/pool_browser_symbol.hpp"
#include "widgets/pool_browser_package.hpp"
#include "widgets/pool_browser_padstack.hpp"

namespace horizon {
	PoolBrowserDialog::PoolBrowserDialog(Gtk::Window *parent, ObjectType type, Pool *ipool) :
		Gtk::Dialog("Select Something", Gtk::DialogFlags::DIALOG_MODAL|Gtk::DialogFlags::DIALOG_USE_HEADER_BAR),
		pool(ipool)
		{
		if(parent) {
			set_transient_for(*parent);
		}
		Gtk::Button *button_ok = add_button("OK", Gtk::ResponseType::RESPONSE_OK);
		add_button("Cancel", Gtk::ResponseType::RESPONSE_CANCEL);
		set_default_response(Gtk::ResponseType::RESPONSE_OK);
		set_default_size(300, 300);
		button_ok->set_sensitive(false);

		switch(type) {
			case ObjectType::UNIT :
				browser = Gtk::manage(new PoolBrowserUnit(pool));
				set_title("Select Unit");
			break;
			case ObjectType::PART :
				browser = Gtk::manage(new PoolBrowserPart(pool));
				set_title("Select Part");
			break;
			case ObjectType::ENTITY :
				browser = Gtk::manage(new PoolBrowserEntity(pool));
				set_title("Select Entity");
			break;
			case ObjectType::SYMBOL :
				browser = Gtk::manage(new PoolBrowserSymbol(pool));
				set_title("Select Symbol");
			break;
			case ObjectType::PACKAGE :
				browser = Gtk::manage(new PoolBrowserPackage(pool));
				set_title("Select Package");
			break;
			case ObjectType::PADSTACK :
				browser = Gtk::manage(new PoolBrowserPadstack(pool));
				set_title("Select Padstack");
			break;
			default:;
		}
		get_content_area()->pack_start(*browser, true, true, 0);
		get_content_area()->set_border_width(0);

		browser->signal_activated().connect([this] {
			response(Gtk::ResponseType::RESPONSE_OK);
		});

		browser->signal_selected().connect([button_ok, this] {
			button_ok->set_sensitive(browser->get_selected());
		});

		get_content_area()->show_all();
	}


	PoolBrowser *PoolBrowserDialog::get_browser() {
		return browser;
	}
}
