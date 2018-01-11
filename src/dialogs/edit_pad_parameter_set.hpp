#pragma once
#include <gtkmm.h>
#include <array>
#include <set>
#include "common/common.hpp"
#include "parameter/set.hpp"

namespace horizon {

	class PadParameterSetDialog: public Gtk::Dialog {
		public:
			PadParameterSetDialog(Gtk::Window *parent, std::set<class Pad*> &pads, class Pool *p, class Package *pkg);
			bool valid = false;


		private :
			class Pool *pool;
			class MyParameterSetEditor *editor = nullptr;
			class PoolBrowserButton *padstack_button = nullptr;
	};
}
