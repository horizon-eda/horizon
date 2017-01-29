#pragma once
#include <gtkmm.h>
#include "canvas/canvas.hpp"

namespace horizon {

	class MainWindow: public Gtk::Window {
		public:
			MainWindow(BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>& x);
			static MainWindow* create();
			CanvasGL *canvas;
			Gtk::Label *active_tool_label;
			Gtk::Label *tool_hint_label;
			Gtk::Label *cursor_label;
			Gtk::Box *left_panel;
			Gtk::Box *top_panel;
			Gtk::Viewport *property_viewport;
			//virtual ~MainWindow();
		private :

			Gtk::Box *gl_container;
			

			void sc(void);
			void cm(const horizon::Coordi &cursor_pos);
			
	};
}
