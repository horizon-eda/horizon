#pragma once
#include <gtkmm.h>
#include <array>
#include <set>
#include "common/common.hpp"
#include "util/uuid.hpp"
namespace horizon {

	class ParameterProgramDialog: public Gtk::Dialog {
		public:
			ParameterProgramDialog(Gtk::Window *parent, class ParameterProgram *pgm);
			bool valid = false;


		private :
			ParameterProgram *program = nullptr;
			Gtk::TextView *tv = nullptr;
			Gtk::InfoBar *bar = nullptr;
			Gtk::Label *bar_label = nullptr;

			void ok_clicked();
	};
}
