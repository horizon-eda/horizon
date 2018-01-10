#include "log_window.hpp"
#include "log_view.hpp"

namespace horizon {
	LogWindow::LogWindow(Gtk::Window *p): Gtk::Window() {
		set_transient_for(*p);
		set_type_hint(Gdk::WINDOW_TYPE_HINT_DIALOG);
		auto hb = Gtk::manage(new Gtk::HeaderBar());
		hb->set_show_close_button(true);
		hb->set_title("Logs");

		set_default_size(800, 300);
		set_titlebar(*hb);
		hb->show_all();

		view = Gtk::manage(new LogView);
		view->signal_logged().connect([this] (const Logger::Item &it) {
			if(open_on_warning && (it.level == Logger::Level::CRITICAL || it.level == Logger::Level::WARNING))
				present();
		});
		add(*view);
		view->show();
	}
}
