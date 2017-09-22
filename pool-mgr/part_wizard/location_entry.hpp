#pragma once
#include <gtkmm.h>

namespace horizon {

	class LocationEntry: public Gtk::Box{
		public:
			LocationEntry();
			void set_filename(const std::string &s);
			std::string get_filename();

		private:
			Gtk::Entry *entry = nullptr;
			void handle_button();

	};
}
