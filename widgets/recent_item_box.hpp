#pragma once
#include <gtkmm.h>

namespace horizon {
	class RecentItemBox: public Gtk::Box {
		public:
			RecentItemBox(const std::string &name, const std::string &path, const Glib::DateTime &time);
			const std::string path;
	};
}
