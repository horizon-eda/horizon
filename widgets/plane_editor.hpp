#pragma once
#include <gtkmm.h>
#include <set>

namespace horizon {
	class PlaneEditor: public Gtk::Grid {
		public:
			PlaneEditor(class PlaneSettings *settings, int *priority = nullptr);
			void set_from_rules(bool v);

		private:
			std::set<Gtk::Widget*> widgets_from_rules_disable;

	};
}
