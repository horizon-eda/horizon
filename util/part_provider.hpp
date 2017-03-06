#pragma once
#include "uuid.hpp"
#include <sigc++/sigc++.h>

namespace horizon {
	class PartProvider {
		public:
			virtual UUID get_selected_part() = 0;
			typedef sigc::signal<void> type_signal_part_selected;
			type_signal_part_selected signal_part_selected() {return s_signal_part_selected;}
			type_signal_part_selected signal_part_activated() {return s_signal_part_activated;}

		protected :
			type_signal_part_selected s_signal_part_selected;
			type_signal_part_selected s_signal_part_activated;
	};
}
