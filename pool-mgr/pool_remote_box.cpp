#include "pool_remote_box.hpp"

namespace horizon {
	PoolRemoteBox::PoolRemoteBox(BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>& x) :
		Gtk::Box(cobject) {
		x->get_widget("button_upgrade_pool", upgrade_button);
		x->get_widget("upgrade_revealer", upgrade_revealer);
		x->get_widget("upgrade_label", upgrade_label);
	}

	PoolRemoteBox* PoolRemoteBox::create() {
		PoolRemoteBox* w;
		Glib::RefPtr<Gtk::Builder> x = Gtk::Builder::create();
		x->add_from_resource("/net/carrotIndustries/horizon/pool-mgr/window.ui", "box_remote");
		x->get_widget_derived("box_remote", w);
		w->reference();
		return w;
	}
}
