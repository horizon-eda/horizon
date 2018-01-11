#pragma once
#include <gtkmm.h>
#include "util/uuid.hpp"

namespace horizon {

	class NetClassButton: public Gtk::ComboBoxText {
		public:
			NetClassButton(class Core *c);
			void set_net_class(const UUID &uu);
			typedef sigc::signal<void, UUID> type_signal_net_class_changed;
			type_signal_net_class_changed signal_net_class_changed() {return s_signal_net_class_changed;}
			void update();

		private :
			class Core *core;
			UUID net_class_current;

			type_signal_net_class_changed s_signal_net_class_changed;
	};

}
