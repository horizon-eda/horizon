#pragma once
#include <gtkmm.h>
#include "core/core.hpp"
#include "common.hpp"

namespace horizon {
	class PropertyEditors: public Gtk::Box {
		public:
			PropertyEditors(const UUID &uu, ObjectType t, Core *c, class PropertyPanel *p);

			const UUID uuid;
			const ObjectType type;
			class PropertyPanel *parent;
			class PropertyEditor *get_editor_for_property(enum ObjectProperty::ID id);
			void reload();
		private:
			Core *core;



	};


}
