#pragma once
#include <gtkmm.h>
#include <array>
#include <set>
#include "common.hpp"
#include "uuid.hpp"
#include "core/core_schematic.hpp"
namespace horizon {


	class AnnotateDialog: public Gtk::Dialog {
		public:
			AnnotateDialog(Gtk::Window *parent, CoreSchematic *c);
			bool valid = false;


		private :
			CoreSchematic *core = nullptr;
			Gtk::Switch *w_fill_gaps = nullptr;
			Gtk::Switch *w_keep = nullptr;
			Gtk::ComboBoxText *w_order = nullptr;
			Gtk::ComboBoxText *w_mode= nullptr;

			void ok_clicked();
	};
}
