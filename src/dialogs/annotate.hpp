#pragma once
#include <gtkmm.h>
#include <array>
#include <set>
#include "common/common.hpp"
#include "util/uuid.hpp"
#include "schematic/schematic.hpp"
namespace horizon {


	class AnnotateDialog: public Gtk::Dialog {
		public:
			AnnotateDialog(Gtk::Window *parent, Schematic *s);
			bool valid = false;


		private :
			Schematic *sch = nullptr;
			Gtk::Switch *w_fill_gaps = nullptr;
			Gtk::Switch *w_keep = nullptr;
			Gtk::ComboBoxText *w_order = nullptr;
			Gtk::ComboBoxText *w_mode= nullptr;

			void ok_clicked();
	};
}
