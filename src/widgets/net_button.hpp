#pragma once
#include <gtkmm.h>
#include "block/block.hpp"
#include "net_selector.hpp"

namespace horizon {

	class NetButton: public Gtk::MenuButton {
		public:
			NetButton(Block *b);
			void set_net(const UUID &uu);
			UUID get_net();
			typedef sigc::signal<void, UUID> type_signal_changed;
			type_signal_changed signal_changed() {return s_signal_changed;}
			void update();

		private :
			Block *block;
			Gtk::Popover *popover;
			NetSelector *ns;
			void update_label();
			void ns_activated(const UUID &uu);
			UUID net_current;
			virtual void on_toggled();

			type_signal_changed s_signal_changed;

	};

}
