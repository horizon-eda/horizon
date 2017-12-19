#pragma once
#include <gtkmm.h>

namespace horizon {
	class PoolRemoteBox: public Gtk::Box {
		public:
			PoolRemoteBox(BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>& x);
			static PoolRemoteBox* create();
			Gtk::Button *upgrade_button = nullptr;
			Gtk::Revealer *upgrade_revealer = nullptr;
			Gtk::Label *upgrade_label = nullptr;


		private :


	};
}
