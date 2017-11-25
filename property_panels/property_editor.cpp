#include "property_editor.hpp"
#include "object_descr.hpp"
#include "property_editors.hpp"
#include "property_panel.hpp"
#include "property_panels.hpp"
#include "block/block.hpp"
#include "core/core_schematic.hpp"
#include "core/core_board.hpp"
#include <algorithm>
#include <iostream>
#include <iomanip>

namespace horizon {
	PropertyEditor::PropertyEditor(const UUID &uu, ObjectType t, ObjectProperty::ID prop, Core *c, class PropertyEditors *p):
		Gtk::Box(Gtk::Orientation::ORIENTATION_VERTICAL, 4),
		parent(p),
		property_id(prop),
		core(c),
		uuid(uu),
		type(t),
		property(object_descriptions.at(type).properties.at(property_id))
	{
		auto *hbox = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL,4));
		auto *header = Gtk::manage(new Gtk::Label());
		header->set_markup("<b>"+property.label+"</b>");
		header->set_halign(Gtk::ALIGN_START);
		hbox->pack_start(*header, true, true, 0);

		apply_all_button = Gtk::manage(new Gtk::Button());
		apply_all_button->set_image_from_icon_name("object-select-symbolic");
		apply_all_button->signal_clicked().connect(sigc::mem_fun(this, &PropertyEditor::handle_apply_all));
		apply_all_button->set_tooltip_text("Apply to all");
		hbox->pack_start(*apply_all_button, false, false, 0);

		pack_start(*hbox, false, false, 0);
	}

	void PropertyEditor::handle_apply_all() {
		auto x = parent->parent->stack;
		for(auto it: x->get_children()) {
			auto pes = dynamic_cast<PropertyEditors*>(it);
			auto pe = pes->get_editor_for_property(property_id);
			if(pe != this) {
				pe->set_from_other(this);
			}
		}
	}

	void PropertyEditor::construct() {
		auto *ed = create_editor();
		ed->set_halign(Gtk::ALIGN_END);
		pack_start(*ed, false, false, 0);
	}

	Gtk::Widget *PropertyEditor::create_editor() {
		Gtk::Label *la = Gtk::manage(new Gtk::Label("fixme"));
		return la;
	}

	Gtk::Widget *PropertyEditorBool::create_editor() {
		sw = Gtk::manage(new Gtk::Switch());
		reload();
		sw->property_active().signal_changed().connect(sigc::mem_fun(this, &PropertyEditorBool::changed));
		return sw;
	}

	void PropertyEditorBool::changed() {
		if(mute)
			return;
		bool st = sw->get_active();
		std::cout << "ch " << st << std::endl;
		core->set_property_bool(uuid,type, property_id, st);
		reload();
		parent->parent->parent->signal_update().emit();
	}

	void PropertyEditorBool::set_from_other(PropertyEditor *other) {
		auto o = dynamic_cast<PropertyEditorBool*>(other);
		sw->set_active(o->sw->get_active());
	}

	void PropertyEditorBool::reload() {
		mute = true;
		sw->set_active(core->get_property_bool(uuid,type, property_id));
		sw->set_sensitive(core->property_is_settable(uuid,type, property_id));
		mute = false;
	}

	Gtk::Widget *PropertyEditorStringRO::create_editor() {
		apply_all_button->set_sensitive(false);
		la = Gtk::manage(new Gtk::Label(core->get_property_string(uuid,type, property_id)));
		return la;
	}

	void PropertyEditorStringRO::reload() {
		la->set_text(core->get_property_string(uuid,type, property_id));
	}

	Gtk::Widget *PropertyEditorLength::create_editor() {
		sp = Gtk::manage(new Gtk::SpinButton());
		sp->set_range(range.first, range.second);
		reload();
		sp->set_increments(.5e6, 10);
		sp->signal_output().connect(sigc::mem_fun(this, &PropertyEditorLength::sp_output));
		sp->signal_input().connect(sigc::mem_fun(this, &PropertyEditorLength::sp_input));
		sp->signal_value_changed().connect(sigc::mem_fun(this, &PropertyEditorLength::changed));
		return sp;
	}

	void PropertyEditorLength::reload() {
		mute = true;
		sp->set_value(core->get_property_int(uuid,type, property_id));
		sp->set_sensitive(core->property_is_settable(uuid,type, property_id));
		mute = false;
	}

	void PropertyEditorLength::set_range(int64_t min, int64_t max) {
		range = {min, max};
	}

	bool PropertyEditorLength::sp_output() {
		auto adj = sp->get_adjustment();
		double value = adj->get_value();

		std::stringstream stream;
		stream << std::fixed << std::setprecision(3) << value/1e6 << " mm";

		sp->set_text(stream.str());
		return true;
	}

	int PropertyEditorLength::sp_input(double *v) {
		auto txt = sp->get_text();
		int64_t va = 0;
		try {
			va = std::stod(txt)*1e6;
			*v = va;
		}
		catch (const std::exception& e) {
			return false;
		}


		return true;
	}

	void PropertyEditorLength::changed() {
		if(mute)
			return;
		int64_t va = sp->get_value_as_int();
		core->set_property_int(uuid,type, property_id, va);
		sp->set_value(core->get_property_int(uuid,type, property_id));
		parent->parent->parent->signal_update().emit();
	}

	void PropertyEditorLength::set_from_other(PropertyEditor *other) {
		auto o = dynamic_cast<PropertyEditorLength*>(other);
		sp->set_value(o->sp->get_value());
	}

	Gtk::Widget *PropertyEditorString::create_editor() {
		en = Gtk::manage(new Gtk::Entry());
		en->set_alignment(1);
		reload();
		en->signal_changed().connect(sigc::mem_fun(this, &PropertyEditorString::changed));
		en->signal_activate().connect(sigc::mem_fun(this, &PropertyEditorString::activate));
		en->signal_focus_out_event().connect(sigc::mem_fun(this, &PropertyEditorString::focus_out_event));
		return en;
	}

	void PropertyEditorString::set_from_other(PropertyEditor *other) {
		auto o = dynamic_cast<PropertyEditorString*>(other);
		en->set_text(o->en->get_text());
		activate();
	}

	void PropertyEditorString::activate() {
		if(!modified)
			return;
		modified = false;
		std::string txt = en->get_text();
		core->set_property_string(uuid,type, property_id, txt);
		parent->parent->parent->signal_update().emit();
		en->unset_icon(Gtk::ENTRY_ICON_PRIMARY);
	}

	bool PropertyEditorString::focus_out_event(GdkEventFocus *e) {
		activate();
		return false;
	}

	void PropertyEditorString::reload() {
		en->set_text(core->get_property_string(uuid,type, property_id));
		en->unset_icon(Gtk::ENTRY_ICON_PRIMARY);
		en->set_sensitive(core->property_is_settable(uuid,type, property_id));
		modified = false;
	}

	void PropertyEditorString::changed() {
		en->set_icon_from_icon_name("content-loading-symbolic", Gtk::ENTRY_ICON_PRIMARY);
		modified = true;
	}

	Gtk::Widget *PropertyEditorLayer::create_editor() {
		combo = Gtk::manage(new Gtk::ComboBoxText());
		for(const auto &it: core->get_layer_provider()->get_layers()) {
			if(!copper_only || it.second.copper)
				combo->insert(0, std::to_string(it.first), it.second.name);
		}
		reload();
		combo->signal_changed().connect(sigc::mem_fun(this, &PropertyEditorLayer::changed));
		return combo;
	}

	void PropertyEditorLayer::reload() {
		mute = true;
		combo->set_active_id(std::to_string(core->get_property_int(uuid, type, property_id)));
		mute = false;
	}

	void PropertyEditorLayer::changed() {
		if(mute)
			return;
		int la = std::stoi(combo->get_active_id());
		core->set_property_int(uuid, type, property_id, la);
		parent->parent->parent->signal_update().emit();
	}

	void PropertyEditorLayer::set_from_other(PropertyEditor *other) {
		auto o = dynamic_cast<PropertyEditorLayer*>(other);
		combo->set_active_id(o->combo->get_active_id());
	}

	Gtk::Widget *PropertyEditorNetClass::create_editor() {
		combo = Gtk::manage(new Gtk::ComboBoxText());
		Block *block = nullptr;
		if(auto c = dynamic_cast<CoreSchematic*>(core)) {
			block = c->get_schematic()->block;
		}
		if(auto c = dynamic_cast<CoreBoard*>(core)) {
			block = c->get_board()->block;
		}
		if(block) {
			for(const auto &it: block->net_classes) {
				combo->insert(0, (std::string)it.first, it.second.name);
			}
		}
		reload();
		combo->signal_changed().connect(sigc::mem_fun(this, &PropertyEditorNetClass::changed));
		return combo;
	}

	void PropertyEditorNetClass::reload() {
		mute = true;
		combo->set_active_id(core->get_property_string(uuid, type, property_id));
		mute = false;
	}

	void PropertyEditorNetClass::changed() {
		if(mute)
			return;
		std::string nc = combo->get_active_id();
		core->set_property_string(uuid, type, property_id, nc);
		parent->parent->parent->signal_update().emit();
	}

	void PropertyEditorNetClass::set_from_other(PropertyEditor *other) {
		auto o = dynamic_cast<PropertyEditorNetClass*>(other);
		combo->set_active_id(o->combo->get_active_id());
	}


	Gtk::Widget *PropertyEditorEnum::create_editor() {
		combo = Gtk::manage(new Gtk::ComboBoxText());
		for(const auto &it: property.enum_items) {
			combo->insert(0, std::to_string(it.first), it.second);
		}
		reload();
		combo->signal_changed().connect(sigc::mem_fun(this, &PropertyEditorEnum::changed));
		return combo;
	}

	void PropertyEditorEnum::reload() {
		mute = true;
		combo->set_active_id(std::to_string(core->get_property_int(uuid, type, property_id)));
		mute = false;
	}

	void PropertyEditorEnum::changed() {
		if(mute)
			return;
		int la = std::stoi(combo->get_active_id());
		core->set_property_int(uuid, type, property_id, la);
		parent->parent->parent->signal_update().emit();
	}

	void PropertyEditorEnum::set_from_other(PropertyEditor *other) {
		auto o = dynamic_cast<PropertyEditorEnum*>(other);
		combo->set_active_id(o->combo->get_active_id());
	}


}
