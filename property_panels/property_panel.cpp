#include "property_panel.hpp"
#include <iostream>
#include "object_descr.hpp"
#include "property_editors.hpp"

namespace horizon {

	PropertyPanel::PropertyPanel(BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>& x) :
		Gtk::Expander(cobject) {
		x->get_widget("stack", stack);
		x->get_widget("selector_label", selector_label);
		x->get_widget("button_prev", button_prev);
		x->get_widget("button_next", button_next);

		button_next->signal_clicked().connect(sigc::bind<int>(sigc::mem_fun(this, &PropertyPanel::go), 1));
		button_prev->signal_clicked().connect(sigc::bind<int>(sigc::mem_fun(this, &PropertyPanel::go), -1));
	}

	PropertyPanel* PropertyPanel::create(ObjectType t, Core *c, PropertyPanels *parent) {
		PropertyPanel* w;
		Glib::RefPtr<Gtk::Builder> x = Gtk::Builder::create();
		x->add_from_resource("/net/carrotIndustries/horizon/property_panels/property_panel.ui");
		x->get_widget_derived("PropertyPanel", w);
		w->reference();
		w->type = t;
		w->core = c;
		w->parent = parent;

		w->set_use_markup(true);
		w->set_label("<b>"+object_descriptions.at(w->type).name_pl+"</b>");
		return w;
	}

	void PropertyPanel::update_selector() {
		auto *cur = stack->get_visible_child();
		auto children = stack->get_children();
		auto f = std::find(children.begin(), children.end(), cur);
		unsigned int i = f-children.begin();

		std::string l = std::to_string(i+1)+"/"+std::to_string(children.size());
		selector_label->set_text(l);
	}

	void PropertyPanel::go(int dir) {
		auto *cur = stack->get_visible_child();
		auto children = stack->get_children();
		auto f = std::find(children.begin(), children.end(), cur);
		int i = f-children.begin();
		i+=dir;
		if(i<0) {
			i+=children.size();
		}
		i %= children.size();
		stack->set_visible_child(*children.at(i));
		update_selector();
	}

	void PropertyPanel::update_objects(const std::set<SelectableRef> &selection) {
		std::map<UUID, PropertyEditors*> editors;
		std::set<UUID> editors_present;
		for(const auto &it: stack->get_children()) {
			auto ed = dynamic_cast<PropertyEditors*>(it);
			editors.emplace(ed->uuid, ed);
			editors_present.emplace(ed->uuid);
		}
		std::set<UUID> editors_from_selection;
		for(const auto &it: selection) {
			editors_from_selection.emplace(it.uuid);
		}
		std::set<UUID> editor_create;
		std::set_difference(editors_from_selection.begin(), editors_from_selection.end(), editors_present.begin(), editors_present.end(),
							std::inserter(editor_create, editor_create.begin()));
		std::set<UUID> editor_delete;
		std::set_difference(editors_present.begin(), editors_present.end(), editors_from_selection.begin(), editors_from_selection.end(),
							std::inserter(editor_delete, editor_delete.begin()));
		for(const auto it: editor_create) {
			std::cout << "create editor " << (std::string)it << std::endl;

			auto *ed = Gtk::manage(new PropertyEditors(it, type, core, this));
			ed->show_all();
			stack->add(*ed);
		}
		for(const auto it: editor_delete) {
			std::cout << "delete editor " << (std::string)it << std::endl;
			delete editors.at(it);
		}
		update_selector();
	}

	ObjectType PropertyPanel::get_type() {
		return type;
	}

	void PropertyPanel::reload() {
		for(const auto &it: stack->get_children()) {
			auto ed = dynamic_cast<PropertyEditors*>(it);
			ed->reload();
		}
	}
}
