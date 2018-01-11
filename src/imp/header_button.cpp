#include "header_button.hpp"
#include "util/gtk_util.hpp"

namespace horizon {
	HeaderButton::HeaderButton(): Gtk::MenuButton() {
		auto box = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL, 6));
		label = Gtk::manage(new Gtk::Label);
		label->get_style_context()->add_class("title");
		box->pack_start(*label, true, true, 0);
		set_label("");

		auto arrow = Gtk::manage(new Gtk::Image);
		arrow->set_from_icon_name("go-down-symbolic", Gtk::ICON_SIZE_BUTTON);
		box->pack_start(*arrow, false, false, 0);

		box->show_all();

		add(*box);
		set_relief(Gtk::RELIEF_NONE);
		auto popover = Gtk::manage(new Gtk::Popover(*this));
		set_popover(*popover);
		grid = Gtk::manage(new Gtk::Grid());
		grid->set_row_spacing(10);
		grid->set_column_spacing(10);
		grid->set_margin_start(20);
		grid->set_margin_end(20);
		grid->set_margin_top(20);
		grid->set_margin_bottom(20);
		popover->add(*grid);
		grid->show();
	}

	void HeaderButton::set_label(const std::string &l) {
		if(!l.size())
			label->set_markup("<i>click here to set name</i>");
		else
			label->set_text(l);
	}

	void HeaderButton::add_widget(const std::string &l, Gtk::Widget *w) {
		grid_attach_label_and_widget(grid, l, w, top);
	}

	Gtk::Entry *HeaderButton::add_entry(const std::string &l) {
		auto entry = Gtk::manage(new Gtk::Entry());
		grid_attach_label_and_widget(grid, l, entry, top);
		return entry;
	}
}
