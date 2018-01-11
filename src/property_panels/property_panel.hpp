#pragma once
#include <gtkmm.h>
#include "common/common.hpp"
#include "canvas/selectables.hpp"
#include "common/object_descr.hpp"
#include <set>

namespace horizon {

	class PropertyPanel: public Gtk::Expander {
		friend class PropertyEditor;
		public:
			PropertyPanel(BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>& x, ObjectType ty, class Core *c);
			static PropertyPanel* create(ObjectType t, Core *c, class PropertyPanels *parent);
			ObjectType get_type();
			void update_objects(const std::set<SelectableRef> &selection);
			class PropertyPanels *parent;
			void reload();

			virtual ~PropertyPanel(){};
		private :
			ObjectType type;
			class Core *core;
			Gtk::MenuButton *button_sel;
			Gtk::Button *button_prev;
			Gtk::Button *button_next;

			Gtk::Menu sel_menu;

			void update_selector();
			void go(int dir);

			Gtk::Box *editors_box = nullptr;
			std::deque<UUID> objects;
			int object_current = 0;

			void handle_changed(ObjectProperty::ID property, const class PropertyValue &value);
			void handle_apply_all(ObjectProperty::ID property, const class PropertyValue &value);

			ObjectProperty::ID last_property;
			gint64 last_property_time;

			sigc::connection throttle_connection;
	};
}
