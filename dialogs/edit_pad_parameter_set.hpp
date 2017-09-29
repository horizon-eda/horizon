#pragma once
#include <gtkmm.h>
#include <array>
#include <set>
#include "common.hpp"
#include "parameter/set.hpp"

namespace horizon {

	class PadParameterSetDialog: public Gtk::Dialog {
		public:
			PadParameterSetDialog(Gtk::Window *parent, std::set<class Pad*> &pads);
			bool valid = false;


		private :
			class MyParameterSetEditor *editor = nullptr;
	};
}
