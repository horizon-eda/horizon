#pragma once
#include <gtkmm.h>
#include <array>
#include <set>
#include "common.hpp"
#include "uuid.hpp"
#include "block.hpp"
namespace horizon {


	class ManagePowerNetsDialog: public Gtk::Dialog {
		public:
			ManagePowerNetsDialog(Gtk::Window *parent, Block *b);

		private :
			Block *block = nullptr;
			Gtk::ListBox *listbox = nullptr;
			void handle_add_power_net();
	};
}
