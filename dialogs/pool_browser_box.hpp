#pragma once
#include <gtkmm.h>

namespace horizon {
	class PoolBrowserBox: public Gtk::Box {
		public:
			PoolBrowserBox(BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>& x);
			static PoolBrowserBox* create();
			Gtk::TreeView *w_treeview;
			Gtk::Entry *w_name_entry;
			Gtk::Entry *w_tags_entry;

		private :


	};

}
