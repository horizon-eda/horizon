#pragma once
#include <gtkmm.h>
#include <array>
#include <set>
#include "common/common.hpp"
#include "parameter/set.hpp"

namespace horizon {

	class BoardHoleDialog: public Gtk::Dialog {
		public:
			BoardHoleDialog(Gtk::Window *parent, std::set<class BoardHole*> &pads, class Pool *p, class Block *block);
			bool valid = false;


		private :
			class Pool *pool;
			class Block *block;
			class MyParameterSetEditor *editor = nullptr;
			class PoolBrowserButton *padstack_button = nullptr;
			class NetButton *net_button = nullptr;
	};
}
