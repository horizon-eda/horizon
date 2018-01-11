#pragma once
#include <gtkmm.h>
#include <array>
#include <set>
#include "common/common.hpp"
#include "parameter/set.hpp"

namespace horizon {

	class ParameterSetDialog: public Gtk::Dialog {
		public:
			ParameterSetDialog(Gtk::Window *parent, ParameterSet *pset);
			bool valid = false;


		private :
			void ok_clicked();
	};
}
