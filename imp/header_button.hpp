#pragma once
#include <gtkmm.h>

namespace horizon {
	class HeaderButton: public Gtk::MenuButton {
		public:
			HeaderButton();
			void set_label(const std::string &l);
			Gtk::Entry *add_entry(const std::string &label);
			void add_widget(const std::string &label, Gtk::Widget *w);

		private:
			int top = 0;
			Gtk::Label *label=nullptr;
			Gtk::Grid *grid = nullptr;


	};
}
