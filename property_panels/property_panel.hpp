#pragma once
#include <gtkmm.h>
#include "common.hpp"
#include "core/core.hpp"

namespace horizon {

	class PropertyPanel: public Gtk::Expander {
		friend class PropertyEditor;
		public:
			PropertyPanel(BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>& x);
			static PropertyPanel* create(ObjectType t, Core *c, class PropertyPanels *parent);
			ObjectType get_type();
			void update_objects(const std::set<SelectableRef> &selection);
			class PropertyPanels *parent;
			void reload();

			virtual ~PropertyPanel(){};
		private :
			ObjectType type;
			Core *core;
			Gtk::Stack *stack;
			Gtk::Label *selector_label;
			Gtk::Button *button_prev;
			Gtk::Button *button_next;

			void update_selector();
			void go(int dir);


	};
}
