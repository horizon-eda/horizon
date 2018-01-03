#include "main_window.hpp"
#include "canvas/canvas_gl.hpp"
#include <iostream>

namespace horizon {

	MainWindow::MainWindow(BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>& x) :
		Gtk::ApplicationWindow(cobject) {

		x->get_widget("gl_container", gl_container);
		x->get_widget("active_tool_label", active_tool_label);
		x->get_widget("tool_hint_label", tool_hint_label);
		x->get_widget("left_panel", left_panel);
		x->get_widget("grid_box", grid_box);
		x->get_widget("grid_mul_label", grid_mul_label);
		x->get_widget("cursor_label", cursor_label);
		x->get_widget("property_viewport", property_viewport);
		x->get_widget("tool_bar", tool_bar);
		x->get_widget("tool_bar_label", tool_bar_name_label);
		x->get_widget("tool_bar_stack", tool_bar_stack);
		x->get_widget("tool_bar_tip", tool_bar_tip_label);
		x->get_widget("tool_bar_flash", tool_bar_flash_label);
		x->get_widget("header", header);
		x->get_widget("property_scrolled_window", property_scrolled_window);
		x->get_widget("property_throttled_revealer", property_throttled_revealer);

		set_icon(Gdk::Pixbuf::create_from_resource("/net/carrotIndustries/horizon/icon.svg"));

		canvas = Gtk::manage(new CanvasGL());
		gl_container->pack_start(*canvas, true, true, 0);
		canvas->show();
		tool_bar_set_visible(false);
		//show_all();
	}

	void MainWindow::tool_bar_set_visible(bool v) {
		if(v==false && tip_timeout_connection) { //hide and tip is still shown
			tool_bar_queue_close = true;
		}
		else {
			tool_bar->set_reveal_child(v);
			if(v) {
				tool_bar_queue_close = false;
			}
		}
	}

	void MainWindow::tool_bar_set_tool_name(const std::string &s) {
		tool_bar_name_label->set_markup(s);
	}

	void MainWindow::tool_bar_set_tool_tip(const std::string &s) {
		tool_bar_tip_label->set_markup(s);
	}

	void MainWindow::tool_bar_flash(const std::string &s) {
		tool_bar_flash_label->set_text(s);
		tool_bar_stack->set_visible_child("flash");
		tip_timeout_connection.disconnect();
		tip_timeout_connection = Glib::signal_timeout().connect([this]{
			tool_bar_stack->set_visible_child("tip");
			if(tool_bar_queue_close)
				tool_bar->set_reveal_child(false);
			tool_bar_queue_close = false;
			return false;
		}, 1000);
	}

	MainWindow* MainWindow::create() {
		MainWindow* w;
		Glib::RefPtr<Gtk::Builder> x = Gtk::Builder::create();
		x->add_from_resource("/net/carrotIndustries/horizon/window.ui");
		x->get_widget_derived("mainWindow", w);
		return w;
	}
}
