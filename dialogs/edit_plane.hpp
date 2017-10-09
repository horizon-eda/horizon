#pragma once
#include <gtkmm.h>
#include <array>
#include <set>
#include "common.hpp"
#include "uuid.hpp"
namespace horizon {
	class EditPlaneDialog: public Gtk::Dialog {
		public:
			EditPlaneDialog(Gtk::Window *parent, class Plane *p, class Board *brd, class Block *b);

		private :
			class Plane *plane = nullptr;

	};
}
