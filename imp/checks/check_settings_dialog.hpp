#pragma once
#include <gtkmm.h>
#include <array>
#include <set>
#include "common.hpp"
#include "uuid.hpp"
namespace horizon {


	class CheckSettingsDialog: public Gtk::Dialog {
		public:
			CheckSettingsDialog(Gtk::Window *parent, class CheckBase *c);

		private :
			CheckBase *check;
			Gtk::Widget *editor_w = nullptr;
			class CheckSettingsEditor *editor = nullptr;
			void ok_clicked();
	};
}
