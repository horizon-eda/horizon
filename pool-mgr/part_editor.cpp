#include "part_editor.hpp"
#include "dialogs/pool_browser_dialog.hpp"
#include "widgets/pool_browser_package.hpp"
#include <iostream>
#include "part.hpp"
#include "util.hpp"
#include <glibmm.h>

namespace horizon {
	class EntryWithInheritance: public Gtk::Box {
		public:
			EntryWithInheritance() :Glib::ObjectBase (typeid(EntryWithInheritance)),  Gtk::Box(Gtk::ORIENTATION_HORIZONTAL, 0),
				p_property_inherit(*this, "inherit"), p_property_can_inherit(*this, "can-inherit") {
				entry = Gtk::manage(new Gtk::Entry);
				button = Gtk::manage(new Gtk::ToggleButton("Inherit"));

				pack_start(*entry, true, true, 0);
				pack_start(*button, false, false, 0);
				get_style_context()->add_class("linked");
				entry->show();
				button->show();

				inh_binding = Glib::Binding::bind_property(button->property_active(), property_inherit(), Glib::BINDING_BIDIRECTIONAL);
				property_inherit().signal_changed().connect([this] {
					entry->set_sensitive(!property_inherit());
					if(property_inherit()) {
						text_this = entry->get_text();
						entry->set_text(text_inherit);
					}
					else {
						entry->set_text(text_this);
					}
				});

				property_can_inherit().signal_changed().connect([this] {
					button->set_sensitive(property_can_inherit());
					if(!property_can_inherit()) {
						property_inherit() = false;
					}
				});

			}
			void set_text_inherit(const std::string &s) {
				text_inherit = s;
				if(property_inherit()) {
					entry->set_text(text_inherit);
				}
			}

			void set_text_this(const std::string &s) {
				text_this = s;
				if(!property_inherit()) {
					entry->set_text(text_this);
				}
			}

			std::pair<bool, std::string> get_as_pair()  {
				if(property_inherit()) {
					return {true, text_this};
				}
				else {
					return {false, entry->get_text()};
				}
			}

			Gtk::Entry *entry=nullptr;
			Gtk::ToggleButton *button = nullptr;


			Glib::PropertyProxy<bool> property_inherit() { return p_property_inherit.get_proxy(); }
			Glib::PropertyProxy<bool> property_can_inherit() { return p_property_can_inherit.get_proxy(); }

		private :
			Glib::Property<bool> p_property_inherit;
			Glib::Property<bool> p_property_can_inherit;
			Glib::RefPtr<Glib::Binding> inh_binding;
			std::string text_inherit;
			std::string text_this;

	};

	static std::string serialize_parametric(const std::map<std::string, std::string> &p) {
		using namespace YAML;
		Emitter em;
		em << BeginMap;
		if(p.count("table")) {
			em << Key << "table" << Value << p.at("table");
		}
		for(const auto &it: p) {
			if(it.first != "table")
				em << Key << it.first << Value << it.second;
		}
		em << EndMap;
		return em.c_str();
	}

	PartEditor::PartEditor(BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>& x, Part *p, Pool *po) :
		Gtk::Box(cobject), part(p), pool(po) {

		auto add_entry = [x](const char *name) {
			Gtk::Box *t;
			x->get_widget(name, t);
			auto e = Gtk::manage(new EntryWithInheritance);
			e->show();
			t->pack_start(*e, true, true, 0);
			return e;
		};

		x->get_widget("entity_label", w_entity_label);
		x->get_widget("package_label", w_package_label);
		x->get_widget("change_package_button", w_change_package_button);
		x->get_widget("base_label", w_base_label);
		x->get_widget("tags", w_tags);
		x->get_widget("tags_inherit", w_tags_inherit);
		x->get_widget("tags_inherited", w_tags_inherited);
		x->get_widget("tv_pins", w_tv_pins);
		x->get_widget("tv_pads", w_tv_pads);
		x->get_widget("button_map", w_button_map);
		x->get_widget("button_unmap", w_button_unmap);
		x->get_widget("pin_stat", w_pin_stat);
		x->get_widget("pad_stat", w_pad_stat);
		x->get_widget("parametric", w_parametric);
		x->get_widget("copy_parametric_from_base", w_parametric_from_base);

		w_mpn = add_entry("part_mpn_box");
		w_value =add_entry("part_value_box");
		w_manufacturer =add_entry("part_manufacturer_box");

		attr_editors.emplace(horizon::Part::Attribute::MPN, w_mpn);
		attr_editors.emplace(horizon::Part::Attribute::VALUE, w_value);
		attr_editors.emplace(horizon::Part::Attribute::MANUFACTURER, w_manufacturer);


		for(auto &it: attr_editors) {
			it.second->property_can_inherit() = part->base;
			it.second->set_text_this(part->attributes.at(it.first).second);
			if(part->base) {
				it.second->set_text_inherit(part->base->attributes.at(it.first).second);
				it.second->property_inherit() = part->attributes.at(it.first).first;
			}
			it.second->entry->signal_changed().connect([this]{needs_save = true;});
			it.second->button->signal_toggled().connect([this]{needs_save = true;});
		}

		update_entries();

		w_change_package_button->signal_clicked().connect(sigc::mem_fun(this, &PartEditor::change_package));

		w_tags_inherit->set_active(part->inherit_tags);
		w_tags_inherit->signal_toggled().connect([this]{needs_save = true;});

		{
			std::stringstream s;
			std::copy(part->tags.begin(), part->tags.end(), std::ostream_iterator<std::string>(s, " "));
			w_tags->set_text(s.str());
			w_tags->signal_changed().connect([this]{needs_save = true;});
		}

		pin_store = Gtk::ListStore::create(pin_list_columns);
		pin_store->set_sort_func(pin_list_columns.pin_name, [this](const Gtk::TreeModel::iterator &ia, const Gtk::TreeModel::iterator &ib) {
			Gtk::TreeModel::Row ra = *ia;
			Gtk::TreeModel::Row rb = *ib;
			Glib::ustring a = ra[pin_list_columns.pin_name];
			Glib::ustring b = rb[pin_list_columns.pin_name];
			return strcmp_natural(a, b);
		});
		w_tv_pins->set_model(pin_store);

		w_tv_pins->append_column("Gate", pin_list_columns.gate_name);
		w_tv_pins->append_column("Pin", pin_list_columns.pin_name);
		{
			auto cr = Gtk::manage(new Gtk::CellRendererPixbuf());
			cr->property_icon_name() = "object-select-symbolic";
			cr->property_xalign() = 0;
			auto tvc = Gtk::manage(new Gtk::TreeViewColumn("Mapped", *cr));
			tvc->add_attribute(cr->property_visible(), pin_list_columns.mapped);
			w_tv_pins->append_column(*tvc);
		}

		w_tv_pins->get_column(0)->set_sort_column(pin_list_columns.gate_name);
		w_tv_pins->get_column(1)->set_sort_column(pin_list_columns.pin_name);

		pin_store->set_sort_column(pin_list_columns.gate_name, Gtk::SORT_ASCENDING);

		pad_store = Gtk::ListStore::create(pad_list_columns);
		pad_store->set_sort_func(pad_list_columns.pad_name, [this](const Gtk::TreeModel::iterator &ia, const Gtk::TreeModel::iterator &ib) {
			Gtk::TreeModel::Row ra = *ia;
			Gtk::TreeModel::Row rb = *ib;
			Glib::ustring a = ra[pad_list_columns.pad_name];
			Glib::ustring b = rb[pad_list_columns.pad_name];
			return strcmp_natural(a, b);
		});
		w_tv_pads->set_model(pad_store);
		w_tv_pads->append_column("Pad", pad_list_columns.pad_name);

		w_tv_pads->append_column("Gate", pad_list_columns.gate_name);
		w_tv_pads->append_column("Pin", pad_list_columns.pin_name);
		w_tv_pads->get_column(0)->set_sort_column(pad_list_columns.pad_name);

		pad_store->set_sort_column(pad_list_columns.pad_name, Gtk::SORT_ASCENDING);

		update_treeview();

		w_button_map->set_sensitive(!part->base);
		w_button_unmap->set_sensitive(!part->base);
		w_change_package_button->set_sensitive(!part->base);

		w_button_unmap->signal_clicked().connect([this] {
			auto sel = w_tv_pads->get_selection();
			for(auto &path: sel->get_selected_rows()) {
				auto it = pad_store->get_iter(path);
				Gtk::TreeModel::Row row = *it;
				row[pad_list_columns.gate_name] = "";
				row[pad_list_columns.pin_name] = "";
				row[pad_list_columns.pin_uuid] = UUID();
				row[pad_list_columns.gate_uuid] = UUID();
			}
			update_mapped();
			needs_save = true;
		});

		w_button_map->signal_clicked().connect([this] {
			auto pin_sel = w_tv_pins->get_selection();
			auto it_pin = pin_sel->get_selected();
			if(it_pin) {
				Gtk::TreeModel::Row row_pin = *it_pin;
				std::cout << "map" << std::endl;
				auto sel = w_tv_pads->get_selection();
				for(auto &path: sel->get_selected_rows()) {
					auto it = pad_store->get_iter(path);
					Gtk::TreeModel::Row row = *it;
					row[pad_list_columns.gate_name] = row_pin.get_value(pin_list_columns.gate_name);
					row[pad_list_columns.pin_name] = row_pin.get_value(pin_list_columns.pin_name);
					row[pad_list_columns.pin_uuid] = row_pin.get_value(pin_list_columns.pin_uuid);
					row[pad_list_columns.gate_uuid] = row_pin.get_value(pin_list_columns.gate_uuid);
				}
				if(++it_pin) {
					pin_sel->select(it_pin);
				}
				if(sel->count_selected_rows() == 1) {
					auto it_pad = pad_store->get_iter(sel->get_selected_rows().at(0));
					sel->unselect(it_pad);
					if(++it_pad) {
						sel->select(it_pad);
					}
				}
				update_mapped();
				needs_save = true;
			}
		});

		update_mapped();

		w_parametric_from_base->set_sensitive(part->base);
		w_parametric->get_buffer()->set_text(serialize_parametric(part->parametric));
		w_parametric->get_buffer()->signal_changed().connect([this]{needs_save=true;});

		w_parametric_from_base->signal_clicked().connect([this] {
			if(part->base) {
				w_parametric->get_buffer()->set_text(serialize_parametric(part->base->parametric));
				needs_save = true;
			}
		});

	}

	void PartEditor::update_entries() {


		if(part->base) {
			w_base_label->set_text(part->base->get_MPN() + " / " + part->base->get_manufacturer());
			w_entity_label->set_text(part->base->entity->name + " / " + part->base->entity->manufacturer);
			w_package_label->set_text(part->base->package->name + " / " + part->base->package->manufacturer);
		}
		else {
			w_base_label->set_text("none");
			w_entity_label->set_text(part->entity->name + " / " + part->entity->manufacturer);
			w_package_label->set_text(part->package->name + " / " + part->package->manufacturer);
		}

		if(part->base) {
			std::stringstream s;
			std::copy(part->base->tags.begin(), part->base->tags.end(), std::ostream_iterator<std::string>(s, " "));
			w_tags_inherited->set_text(s.str());
		}
		else {
			w_tags_inherited->set_text("");
		}

		w_tags_inherit->set_sensitive(part->base);
	}

	void PartEditor::change_package() {
		auto top = dynamic_cast<Gtk::Window*>(get_ancestor(GTK_TYPE_WINDOW));
		PoolBrowserDialog dia(top, ObjectType::PACKAGE, pool);
		if(dia.run() == Gtk::RESPONSE_OK) {
			needs_save = true;
			part->package = pool->get_package(dia.get_browser()->get_selected());
			auto ch = pad_store->children();
			std::set<UUID> pads_exisiting;
			for(auto it = ch.begin(); it!=ch.end();) {
				Gtk::TreeModel::Row row = *it;
				auto pad_name = row[pad_list_columns.pad_name];
				UUID pad_uuid;
				for(const auto &it_pad: part->package->pads) {
					if(it_pad.second.name == pad_name) {
						pad_uuid = it_pad.second.uuid;
						break;
					}
				}
				if(pad_uuid) {
					row[pad_list_columns.pad_uuid] = pad_uuid;
					pads_exisiting.insert(pad_uuid);
					it++;
				}
				else {
					pad_store->erase(it++);
				}
			}
			for(const auto &it: part->package->pads) {
				if(pads_exisiting.count(it.first) == 0) {
					Gtk::TreeModel::Row row = *(pad_store->append());
					row[pad_list_columns.pad_uuid] = it.first;
					row[pad_list_columns.pad_name] = it.second.name;
				}
			}

			update_entries();
			update_mapped();
		}
	}

	void PartEditor::reload() {
		part->update_refs(*pool);
		update_entries();
	}

	void PartEditor::save() {
		for(auto &it: attr_editors) {
			part->attributes[it.first] = it.second->get_as_pair();
		}
		{
			std::stringstream ss(w_tags->get_text());
			std::istream_iterator<std::string> begin(ss);
			std::istream_iterator<std::string> end;
			std::vector<std::string> tags(begin, end);
			part->tags.clear();
			part->tags.insert(tags.begin(), tags.end());
		}
		part->inherit_tags = w_tags_inherit->get_active();

		part->parametric.clear();
		auto para = YAML::Load(w_parametric->get_buffer()->get_text());
		for(const auto &it: para) {
			part->parametric.emplace(it.first.as<std::string>(), it.second.as<std::string>());
		}

		part->pad_map.clear();
		for(const auto &it: pad_store->children()) {
			if(it[pad_list_columns.gate_uuid] != UUID() && part->package->pads.count(it[pad_list_columns.pad_uuid])) {
				const horizon::Gate *gate = &part->entity->gates.at(it[pad_list_columns.gate_uuid]);
				const horizon::Pin *pin = &gate->unit->pins.at(it[pad_list_columns.pin_uuid]);
				part->pad_map.emplace(it[pad_list_columns.pad_uuid], horizon::Part::PadMapItem(gate, pin));
			}
		}
		needs_save = false;
	}


	void PartEditor::update_treeview() {
		pin_store->freeze_notify();
		pad_store->freeze_notify();

		pin_store->clear();
		pad_store->clear();

		for(const auto &it_gate: part->entity->gates) {
			for(const auto &it_pin: it_gate.second.unit->pins) {
				Gtk::TreeModel::Row row = *(pin_store->append());
				row[pin_list_columns.gate_uuid] = it_gate.first;
				row[pin_list_columns.gate_name] = it_gate.second.name;
				row[pin_list_columns.pin_uuid] = it_pin.first;
				row[pin_list_columns.pin_name] = it_pin.second.primary_name;
			}
		}

		for(const auto &it: part->package->pads) {
			if(it.second.pool_padstack->type != Padstack::Type::MECHANICAL) {
				Gtk::TreeModel::Row row = *(pad_store->append());
				row[pad_list_columns.pad_uuid] = it.first;
				row[pad_list_columns.pad_name] = it.second.name;
				if(part->pad_map.count(it.first)) {
					const auto &x =part->pad_map.at(it.first);
					row[pad_list_columns.gate_uuid] = x.gate->uuid;
					row[pad_list_columns.gate_name] = x.gate->name;
					row[pad_list_columns.pin_uuid] = x.pin->uuid;
					row[pad_list_columns.pin_name] = x.pin->primary_name;
				}
			}
		}

		pad_store->thaw_notify();
		pin_store->thaw_notify();
	}

	void PartEditor::update_mapped() {
		std::set<std::pair<UUID, UUID>> pins_mapped;
		int n_pads_not_mapped = 0;
		for(const auto &it: pad_store->children()) {
			if(it[pad_list_columns.gate_uuid] != UUID()) {
				pins_mapped.emplace(it[pad_list_columns.gate_uuid], it[pad_list_columns.pin_uuid]);
			}
			else {
				n_pads_not_mapped++;
			}
		}
		for(auto &it: pin_store->children()) {
			if(pins_mapped.count({it[pin_list_columns.gate_uuid], it[pin_list_columns.pin_uuid]})) {
				it[pin_list_columns.mapped] = true;
			}
			else {
				it[pin_list_columns.mapped] = false;
			}
		}
		w_pin_stat->set_text(std::to_string(pin_store->children().size()-pins_mapped.size()) + " pins not mapped");
		w_pad_stat->set_text(std::to_string(n_pads_not_mapped) + " pads not mapped");
	}

	PartEditor* PartEditor::create(Part *p, Pool *po) {
		PartEditor* w;
		Glib::RefPtr<Gtk::Builder> x = Gtk::Builder::create();
		x->add_from_resource("/net/carrotIndustries/horizon/pool-mgr/part_editor.ui");
		x->get_widget_derived("part_editor", w, p, po);
		w->reference();
		return w;
	}
}
