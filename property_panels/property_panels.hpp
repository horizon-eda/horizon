#pragma once
#include <gtkmm.h>
#include "core/core.hpp"

namespace horizon {
	class PropertyPanels: public Gtk::Box {
		public:
			PropertyPanels(Core *c);
			void update_objects(const std::set<SelectableRef> &selection);
			void reload();
			typedef sigc::signal<void> type_signal_update;
			type_signal_update signal_update() {return s_signal_update;}


		private:
			Core *core;
			type_signal_update s_signal_update;
	};


}
