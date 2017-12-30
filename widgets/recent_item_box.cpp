#include "recent_item_box.hpp"

namespace horizon {
	RecentItemBox::RecentItemBox(const std::string &name, const std::string &pa, const Glib::DateTime &time): Gtk::Box(Gtk::ORIENTATION_VERTICAL, 6), path(pa) {
		property_margin() = 12;
		auto tbox = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL, 12));
		{
			auto la = Gtk::manage(new Gtk::Label());
			la->set_markup("<b>"+name+"</b>");
			la->set_xalign(0);
			tbox->pack_start(*la, true, true, 0);
		}
		{
			auto la = Gtk::manage(new Gtk::Label(time.format("%c")));
			tbox->pack_start(*la, false, false, 0);
		}
		pack_start(*tbox, true, true, 0);
		{
			auto la = Gtk::manage(new Gtk::Label(path));
			la->set_xalign(0);
			la->get_style_context()->add_class("dim-label");
			pack_start(*la, false, false, 0);
		}
		show_all();
	}
}
