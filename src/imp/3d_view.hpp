#pragma once
#include <gtkmm.h>
#include "util/window_state_store.hpp"

namespace horizon {
	class View3DWindow: public Gtk::Window {
		public:
			View3DWindow(BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>& x, const class Board *b, class Pool *p);
			static View3DWindow* create(const class Board *b, class Pool *p);
			void update(bool clear=false);
			void add_widget(Gtk::Widget *w);

			typedef sigc::signal<void> type_signal_request_update;
			type_signal_request_update signal_request_update() {return s_signal_request_update;}

			bool from_pool = true;

		private:
			class Canvas3D *canvas = nullptr;
			const class Board *board;
			class Pool *pool = nullptr;
			Gtk::Box *main_box = nullptr;

			Gtk::ColorButton *background_top_color_button = nullptr;
			Gtk::ColorButton *background_bottom_color_button = nullptr;
			Gtk::ComboBoxText *background_color_preset_combo = nullptr;
			bool setting_background_color_from_preset = false;

			WindowStateStore state_store;

			type_signal_request_update s_signal_request_update;
	};
}
