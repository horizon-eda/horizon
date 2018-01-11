#include "property_panels.hpp"
#include "property_panel.hpp"
#include "common/object_descr.hpp"
#include <algorithm>
#include <iostream>

namespace horizon {
	PropertyPanels::PropertyPanels(Core *c):
		Gtk::Box(Gtk::Orientation::ORIENTATION_VERTICAL, 16),
		core(c)
	{

	}

	void PropertyPanels::update_objects(const std::set<SelectableRef> &selection) {
		std::set<Gtk::Widget*> to_delete;
		std::set<ObjectType> types;
		std::set<ObjectType> types_present;
		for(const auto &it: selection) {
			types.emplace(it.type);
		}
		std::map<ObjectType, PropertyPanel*> children;
		for(const auto &it: get_children()) {
			auto pp = dynamic_cast<PropertyPanel*>(it);
			types_present.emplace(pp->get_type());
			children.emplace(pp->get_type(), pp);
		}

		std::set<ObjectType> panel_create;
		std::set_difference(types.begin(), types.end(), types_present.begin(), types_present.end(),
		                    std::inserter(panel_create, panel_create.begin()));
		std::set<ObjectType> panel_delete;
		std::set_difference(types_present.begin(), types_present.end(), types.begin(), types.end(),
							std::inserter(panel_delete, panel_delete.begin()));
		for(const auto it: panel_create) {
			if(object_descriptions.count(it)) {
				if(object_descriptions.at(it).properties.size()>0) {
					auto *pp = PropertyPanel::create(it, core, this);
					pp->show_all();
					pack_start(*pp, false, false, 0);
					pp->unreference();
				}
			}
		}
		for(const auto it: panel_delete) {
			std::cout << "delete panel " << object_descriptions.at(it).name << std::endl;
			delete children.at(it);
		}

		selection_stored.clear();
		for(const auto &it: get_children()) {
			auto pp = dynamic_cast<PropertyPanel*>(it);
			auto ty = pp->get_type();
			std::set<SelectableRef> sel_this;
			std::copy_if(selection.begin(), selection.end(), std::inserter(sel_this, sel_this.begin()), [ty](const auto &x){return x.type==ty;});
			pp->update_objects(sel_this);
			selection_stored.insert(sel_this.begin(), sel_this.end());
		}
	}

	void PropertyPanels::reload() {
		for(const auto &it: get_children()) {
			auto pp = dynamic_cast<PropertyPanel*>(it);
			pp->reload();
		}
	}

	void PropertyPanels::set_property(ObjectType ty, const UUID &uu, ObjectProperty::ID property, const class PropertyValue &value) {
		if(!core->get_property_transaction()) {
			core->set_property_begin();
			s_signal_throttled.emit(true);
		}
		core->set_property(ty, uu, property, value);
		s_signal_update.emit();
		throttle_connection.disconnect(); //stop old timer
		throttle_connection = Glib::signal_timeout().connect([this]{
			core->set_property_commit();
			s_signal_update.emit();
			s_signal_throttled.emit(false);
			reload();
			return false;
		}, 700);
	}

	void PropertyPanels::force_commit() {
		assert(core->get_property_transaction());
		throttle_connection.disconnect();
		core->set_property_commit();
		s_signal_update.emit();
		s_signal_throttled.emit(false);
		reload();
	}
}
