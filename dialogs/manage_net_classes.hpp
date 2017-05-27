#pragma once
#include <gtkmm.h>
#include <array>
#include <set>
#include "common.hpp"
#include "uuid.hpp"
#include "block.hpp"
namespace horizon {


	class ManageNetClassesDialog: public Gtk::Dialog {
		public:
			ManageNetClassesDialog(Gtk::Window *parent, Block *b);
			bool valid = false;

		private :
			Block *block = nullptr;
			Gtk::ListBox *listbox = nullptr;
			void handle_add_net_class();

			void ok_clicked();
	};
}
