#include "pool_browser_box.hpp"

namespace horizon {

	PoolBrowserBox::PoolBrowserBox(BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>& x) :
		Gtk::Box(cobject) {

		show_all();
	}

	PoolBrowserBox* PoolBrowserBox::create() {
		PoolBrowserBox* w;
		Glib::RefPtr<Gtk::Builder> x = Gtk::Builder::create();
		x->add_from_resource("/net/carrotIndustries/horizon/dialogs/pool_browser.ui");
		x->get_widget_derived("browser_box", w);
		x->get_widget("treeview", w->w_treeview);
		x->get_widget("name_entry", w->w_name_entry);
		x->get_widget("tags_entry", w->w_tags_entry);

		w->reference();
		return w;
	}


}
