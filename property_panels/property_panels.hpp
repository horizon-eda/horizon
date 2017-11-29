#pragma once
#include <gtkmm.h>
#include "core/core.hpp"
#include "canvas/selectables.hpp"

namespace horizon {
	class PropertyPanels: public Gtk::Box {
		friend class PropertyPanel;
		public:
			PropertyPanels(Core *c);
			void update_objects(const std::set<SelectableRef> &selection);
			void reload();
			typedef sigc::signal<void> type_signal_update;
			type_signal_update signal_update() {return s_signal_update;}

			typedef sigc::signal<void, bool> type_signal_throttled;
			type_signal_throttled signal_throttled() {return s_signal_throttled;}

			const std::set<SelectableRef> &get_selection() const {return selection_stored;}

		private:
			Core *core;
			type_signal_update s_signal_update;
			type_signal_throttled s_signal_throttled;
			std::set<SelectableRef> selection_stored;

			void set_property(ObjectType ty, const UUID &uu, ObjectProperty::ID property, const class PropertyValue &value);
			sigc::connection throttle_connection;
			void force_commit();
	};


}
