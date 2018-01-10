#include "parameter_window.hpp"
#include "widgets/parameter_set_editor.hpp"

namespace horizon {
	ParameterWindow::ParameterWindow(Gtk::Window *p, std::string *ppc, ParameterSet *ps): Gtk::Window() {
		set_transient_for(*p);
		set_type_hint(Gdk::WINDOW_TYPE_HINT_DIALOG);
		auto hb = Gtk::manage(new Gtk::HeaderBar());
		hb->set_show_close_button(true);
		hb->set_title("Parameters");

		apply_button = Gtk::manage(new Gtk::Button("Apply"));
		apply_button->get_style_context()->add_class("suggested-action");
		apply_button->signal_clicked().connect([this]{signal_apply().emit();});
		hb->pack_start(*apply_button);

		set_default_size(800, 300);
		set_titlebar(*hb);
		hb->show_all();


		auto box2 = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_VERTICAL,0)); //for info bar
		auto box = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL,0));

		auto tbox = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_VERTICAL, 0));
		extra_button_box = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL, 10));
		extra_button_box->set_margin_start(10);
		extra_button_box->set_margin_end(10);
		extra_button_box->set_margin_top(10);
		extra_button_box->set_margin_bottom(10);

		tbox->pack_start(*extra_button_box, false, false, 0);
		auto sc = Gtk::manage(new Gtk::ScrolledWindow());
		tv = Gtk::manage(new Gtk::TextView());
		tv->get_buffer()->set_text(*ppc);
		tv->set_monospace(true);
		tv->get_buffer()->signal_changed().connect([ppc, this] {*ppc = tv->get_buffer()->get_text();});
		sc->add(*tv);
		tbox->pack_start(*sc, true, true, 0);
		box->pack_start(*tbox, true, true, 0);

		auto sep = Gtk::manage(new Gtk::Separator());
		box->pack_start(*sep, false, false, 0);

		auto editor = Gtk::manage(new ParameterSetEditor(ps));
		box->pack_start(*editor, false, false, 0);

		box2->pack_start(*box, true, true, 0);

		bar = Gtk::manage(new Gtk::InfoBar());
		bar_label = Gtk::manage(new Gtk::Label("fixme"));
		bar_label->set_xalign(0);
		dynamic_cast<Gtk::Box*>(bar->get_content_area())->pack_start(*bar_label, true, true, 0);
		box2->pack_start(*bar, false, false, 0);

		box2->show_all();
		add(*box2);
		bar->hide();
		extra_button_box->hide();
	}

	void ParameterWindow::set_can_apply(bool v) {
		apply_button->set_sensitive(v);
	}

	void ParameterWindow::add_button(Gtk::Widget *button) {
		extra_button_box->pack_start(*button, false, false, 0);
		extra_button_box->show_all();
	}

	void ParameterWindow::insert_text(const std::string &text) {
		tv->get_buffer()->insert_at_cursor(text);
	}

	void ParameterWindow::set_error_message(const std::string &s) {
		if(s.size()) {
			bar->show();
			bar_label->set_markup(s);
			bar->set_size_request(0,0);
			bar->set_size_request(-1,-1);
		}
		else {
			bar->hide();
		}
	}
}
