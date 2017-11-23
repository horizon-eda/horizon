#pragma once
#include <gtkmm.h>
#include "util/window_state_store.hpp"

namespace horizon {
	class View3DWindow: public Gtk::Window {
		public:
			View3DWindow(BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>& x, const class Board *b);
			static View3DWindow* create(const class Board *b);
			void update();

		private:
			class Canvas3D *canvas = nullptr;
			const class Board *board;

			WindowStateStore state_store;
	};
}
