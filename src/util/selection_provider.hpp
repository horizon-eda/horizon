#pragma once
#include "util/uuid.hpp"
#include <sigc++/sigc++.h>

namespace horizon {
	class SelectionProvider {
		public:
			virtual UUID get_selected() = 0;
			typedef sigc::signal<void> type_signal_selected;
			type_signal_selected signal_selected() {return s_signal_selected;}
			type_signal_selected signal_activated() {return s_signal_activated;}

		protected :
			type_signal_selected s_signal_selected;
			type_signal_selected s_signal_activated;
	};
}
