#pragma once
#include <gtkmm.h>
#include <set>
#include "util/uuid.hpp"
#include "common/common.hpp"

namespace horizon {
	class PreviewBase {
		public:
			typedef sigc::signal<void, ObjectType, UUID> type_signal_goto;
			type_signal_goto signal_goto() {return s_signal_goto;}

		protected:

			Gtk::Button *create_goto_button(ObjectType type, std::function<UUID(void)> fn);
			std::set<Gtk::Button*> goto_buttons;
			type_signal_goto s_signal_goto;

	};
}
