#pragma once
#include <gtkmm.h>

namespace horizon {

	class LocationEntry: public Gtk::Box{
		public:
			LocationEntry();
			void set_filename(const std::string &s);
			std::string get_filename();
			void set_warning(const std::string &t);

			typedef sigc::signal<void> type_signal_changed;
			type_signal_changed signal_changed() {return s_signal_changed;}

		private:
			Gtk::Entry *entry = nullptr;
			void handle_button();

			type_signal_changed s_signal_changed;

	};
}
