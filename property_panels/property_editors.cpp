#include "property_editors.hpp"
#include "property_editor.hpp"
#include "object_descr.hpp"
#include <algorithm>
#include <iostream>

namespace horizon {
	PropertyEditors::PropertyEditors(const UUID &uu, ObjectType t, Core *c, PropertyPanel *p):
		Gtk::Box(Gtk::Orientation::ORIENTATION_VERTICAL, 16),
		uuid(uu),
		type(t),
		parent(p),
		core(c)
	{
		for(const auto &it: object_descriptions.at(type).properties) {
			PropertyEditor *e;
			switch(it.second.type) {
				case ObjectProperty::Type::BOOL :
					e = new PropertyEditorBool(uuid, type, it.first, core, this);
				break;
				case ObjectProperty::Type::STRING_RO:
					e = new PropertyEditorStringRO(uuid, type, it.first, core, this);
				break;
				case ObjectProperty::Type::LENGTH:
					e = new PropertyEditorLength(uuid, type, it.first, core, this);
				break;
				case ObjectProperty::Type::STRING:
					e = new PropertyEditorString(uuid, type, it.first, core, this);
				break;
				case ObjectProperty::Type::LAYER:
					e = new PropertyEditorLayer(uuid, type, it.first, core, this);
				break;
				case ObjectProperty::Type::LAYER_COPPER: {
					auto pe = new PropertyEditorLayer(uuid, type, it.first, core, this);
					pe->copper_only = true;
					e = pe;
				} break;
				case ObjectProperty::Type::NET_CLASS:
					e = new PropertyEditorNetClass(uuid, type, it.first, core, this);
				break;
				case ObjectProperty::Type::ENUM:
					e = new PropertyEditorEnum(uuid, type, it.first, core, this);
				break;

				default :
					e = new PropertyEditor(uuid, type, it.first, core, this);
			}
			auto em = Gtk::manage(e);
			em->construct();
			pack_start(*em, false, false, 0);
		}
	}

	PropertyEditor *PropertyEditors::get_editor_for_property(ObjectProperty::ID id) {
		for(auto it: get_children()) {
			auto pe = dynamic_cast<PropertyEditor*>(it);
			if(pe->property_id == id) {
				return pe;
			}
		}
		return nullptr;
	}

	void PropertyEditors::reload() {
		for(auto it: get_children()) {
			auto pe = dynamic_cast<PropertyEditor*>(it);
			pe->reload();
		}
	}
}
