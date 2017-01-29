#include "main_window.hpp"
#include <iostream>

namespace horizon {

	MainWindow::MainWindow(BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>& x) :
		Gtk::Window(cobject) {

		x->get_widget("gl_container", gl_container);
		x->get_widget("active_tool_label", active_tool_label);
		x->get_widget("tool_hint_label", tool_hint_label);
		x->get_widget("left_panel", left_panel);
		x->get_widget("top_panel", top_panel);
		x->get_widget("cursor_label", cursor_label);
		x->get_widget("property_viewport", property_viewport);
		canvas = Gtk::manage(new CanvasGL());
		gl_container->pack_start(*canvas, true, true, 0);
		show_all();
	}

	MainWindow* MainWindow::create() {
		MainWindow* w;
		Glib::RefPtr<Gtk::Builder> x = Gtk::Builder::create();
		x->add_from_resource("/net/carrotIndustries/horizon/window.ui");
		x->get_widget_derived("mainWindow", w);
		return w;
	}
}
