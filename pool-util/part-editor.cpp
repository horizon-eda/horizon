#include "part-editor.hpp"
#include "dialogs/pool_browser_entity.hpp"
#include "dialogs/pool_browser_package.hpp"
#include "dialogs/pool_browser_part.hpp"
#include "util.hpp"
#include <iterator>

using json = nlohmann::json;


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

class PartEditorWindow: public Gtk::Window {
	public:
		PartEditorWindow(BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>& x);
		static PartEditorWindow* create();
		EntryWithInheritance *w_mpn;
		EntryWithInheritance *w_manufacturer;
		EntryWithInheritance *w_value;

		std::map<horizon::Part::Attribute, EntryWithInheritance*> attr_editors;

		Gtk::Entry *w_tags;
		Gtk::Entry *w_tags_inherited;
		Gtk::ToggleButton *w_tags_inherit;
		Gtk::Box *w_entity_button_box;
		Gtk::Box *w_package_button_box;
		Gtk::Box *w_part_button_box;
		Gtk::TreeView *w_tv_pins;
		Gtk::TreeView *w_tv_pads;
		Gtk::Button *w_button_map;
		Gtk::Button *w_button_unmap;
		Gtk::Button *w_button_save;
		Gtk::Label *w_pin_stat;
		Gtk::Label *w_pad_stat;
		Gtk::TextView *w_parametric;
		Gtk::Button *w_parametric_from_base;

	private :


};


PartEditorWindow::PartEditorWindow(BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>& x) :
	Gtk::Window(cobject) {

	show_all();
}

PartEditorWindow* PartEditorWindow::create() {
	PartEditorWindow* w;
	Glib::RefPtr<Gtk::Builder> x = Gtk::Builder::create();
	x->add_from_resource("/net/carrotIndustries/horizon/pool-util/part-editor.ui");
	x->get_widget_derived("window", w);

	auto add_entry = [x, w](const char *name) {
		Gtk::Box *t;
		x->get_widget(name, t);
		auto e = Gtk::manage(new EntryWithInheritance);
		e->show();
		t->pack_start(*e, true, true, 0);
		return e;
	};

	w->w_mpn = add_entry("part_mpn_box");
	w->w_value =add_entry("part_value_box");
	w->w_manufacturer =add_entry("part_manufacturer_box");
	w->attr_editors.emplace(horizon::Part::Attribute::MPN, w->w_mpn);
	w->attr_editors.emplace(horizon::Part::Attribute::VALUE, w->w_value);
	w->attr_editors.emplace(horizon::Part::Attribute::MANUFACTURER, w->w_manufacturer);

	x->get_widget("part_tags", w->w_tags);
	x->get_widget("part_tags_inherit", w->w_tags_inherit);
	x->get_widget("part_tags_inherited", w->w_tags_inherited);
	x->get_widget("entity_button_box", w->w_entity_button_box);
	x->get_widget("package_button_box", w->w_package_button_box);
	x->get_widget("part_button_box", w->w_part_button_box);
	x->get_widget("tv_pins", w->w_tv_pins);
	x->get_widget("tv_pads", w->w_tv_pads);
	x->get_widget("button_map", w->w_button_map);
	x->get_widget("button_unmap", w->w_button_unmap);
	x->get_widget("button_save", w->w_button_save);
	x->get_widget("pad_stat", w->w_pad_stat);
	x->get_widget("pin_stat", w->w_pin_stat);
	x->get_widget("parametric", w->w_parametric);
	x->get_widget("copy_parametric_from_base", w->w_parametric_from_base);
	return w;
}



PartEditor::PartEditor(const std::string &fn, bool create): filename(fn), pool(Glib::getenv("HORIZON_POOL")), part(create?horizon::UUID::random():horizon::Part::new_from_file(filename, pool)) {}


class EntityButton: public Gtk::Button {
	public:
		EntityButton(horizon::Pool &p):Glib::ObjectBase (typeid(EntityButton)),  Gtk::Button("fixme"), p_property_selected_uuid(*this, "selected-uuid"), pool(p) {
			update_label();
			property_selected_uuid().signal_changed().connect([this]{update_label();});
		}
		Glib::PropertyProxy<horizon::UUID> property_selected_uuid() { return p_property_selected_uuid.get_proxy(); }

	protected :
		Glib::Property<horizon::UUID> p_property_selected_uuid;
		horizon::Pool &pool;

		void on_clicked() override {
			Gtk::Button::on_clicked();
			auto top = dynamic_cast<Gtk::Window*>(get_ancestor(GTK_TYPE_WINDOW));
			horizon::PoolBrowserDialogEntity pb(top, &pool);
			pb.run();
			if(pb.selection_valid) {
				p_property_selected_uuid = pb.selected_uuid;
				update_label();
			}
		}

		void update_label() {
			horizon::UUID uu = p_property_selected_uuid;
			if(uu) {
				set_label(pool.get_entity(uu)->name);
			}
			else {
				set_label("no entity");
			}
		}
};

class PackageButton: public Gtk::Button {
	public:
		PackageButton(horizon::Pool &p):Glib::ObjectBase (typeid(PackageButton)),  Gtk::Button("fixme"), p_property_selected_uuid(*this, "selected-uuid"), pool(p) {
			update_label();
			property_selected_uuid().signal_changed().connect([this]{update_label();});
		}
		Glib::PropertyProxy<horizon::UUID> property_selected_uuid() { return p_property_selected_uuid.get_proxy(); }

	protected :
		Glib::Property<horizon::UUID> p_property_selected_uuid;
		horizon::Pool &pool;

		void on_clicked() override {
			Gtk::Button::on_clicked();
			auto top = dynamic_cast<Gtk::Window*>(get_ancestor(GTK_TYPE_WINDOW));
			horizon::PoolBrowserDialogPackage pb(top, &pool);
			pb.run();
			if(pb.selection_valid) {
				p_property_selected_uuid = pb.selected_uuid;
				update_label();
			}
		}

		void update_label() {
			horizon::UUID uu = p_property_selected_uuid;
			if(uu) {
				set_label(pool.get_package(uu)->name);
			}
			else {
				set_label("no package");
			}
		}
};

class PartButton: public Gtk::Button {
	public:
		PartButton(horizon::Pool &p):Glib::ObjectBase (typeid(PartButton)),  Gtk::Button("fixme"), p_property_selected_uuid(*this, "selected-uuid"), pool(p) {
			update_label();
			property_selected_uuid().signal_changed().connect([this]{update_label();});
		}
		Glib::PropertyProxy<horizon::UUID> property_selected_uuid() { return p_property_selected_uuid.get_proxy(); }

	protected :
		Glib::Property<horizon::UUID> p_property_selected_uuid;
		horizon::Pool &pool;

		void on_clicked() override {
			Gtk::Button::on_clicked();
			auto top = dynamic_cast<Gtk::Window*>(get_ancestor(GTK_TYPE_WINDOW));
			horizon::PoolBrowserDialogPart pb(top, &pool, horizon::UUID());
			pb.run();
			if(pb.selection_valid) {
				p_property_selected_uuid = pb.selected_uuid;
				update_label();
			}
		}

		void update_label() {
			horizon::UUID uu = p_property_selected_uuid;
			if(uu) {
				set_label(pool.get_part(uu)->get_MPN());
			}
			else {
				set_label("no part");
			}
		}
};

void PartEditor::update_mapped() {
	std::set<std::pair<horizon::UUID, horizon::UUID>> pins_mapped;
	int n_pads_not_mapped = 0;
	for(const auto &it: pad_store->children()) {
		if(it[pad_list_columns.gate_uuid] != horizon::UUID()) {
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
	win->w_pin_stat->set_text(std::to_string(pin_store->children().size()-pins_mapped.size()) + " pins not mapped");
	win->w_pad_stat->set_text(std::to_string(n_pads_not_mapped) + " pads not mapped");
}

void PartEditor::update_treeview() {
	if(!part.entity || !part.package)
		return;

	pin_store->freeze_notify();
	pad_store->freeze_notify();

	pin_store->clear();
	pad_store->clear();

	for(const auto &it_gate: part.entity->gates) {
		for(const auto &it_pin: it_gate.second.unit->pins) {
			Gtk::TreeModel::Row row = *(pin_store->append());
			row[pin_list_columns.gate_uuid] = it_gate.first;
			row[pin_list_columns.gate_name] = it_gate.second.name;
			row[pin_list_columns.pin_uuid] = it_pin.first;
			row[pin_list_columns.pin_name] = it_pin.second.primary_name;
		}
	}

	for(const auto &it: part.package->pads) {
		Gtk::TreeModel::Row row = *(pad_store->append());
		row[pad_list_columns.pad_uuid] = it.first;
		row[pad_list_columns.pad_name] = it.second.name;
		if(part.pad_map.count(it.first)) {
			const auto &x =part.pad_map.at(it.first);
			row[pad_list_columns.gate_uuid] = x.gate->uuid;
			row[pad_list_columns.gate_name] = x.gate->name;
			row[pad_list_columns.pin_uuid] = x.pin->uuid;
			row[pad_list_columns.pin_name] = x.pin->primary_name;
		}
	}

	pad_store->thaw_notify();
	pin_store->thaw_notify();

	update_mapped();
}

static void load_entry_base(EntryWithInheritance *e, horizon::Part *part, horizon::Part::Attribute a) {
	if(part->base) {
		e->set_text_inherit(part->base->get_attribute(a));
	}
}


static void load_entry(EntryWithInheritance *e, horizon::Part *part, horizon::Part::Attribute a) {
	e->set_text_this(part->attributes.at(a).second);
	e->property_inherit() = part->attributes.at(a).first;
	load_entry_base(e, part, a);
}

void PartEditor::update_tags_inherit() {
	if(part.base) {
		std::stringstream s;
		std::copy(part.base->tags.begin(), part.base->tags.end(), std::ostream_iterator<std::string>(s, " "));
		win->w_tags_inherited->set_text(s.str());
	}
	else {
		win->w_tags_inherited->set_text("");
	}
	win->w_tags_inherit->set_active(part.inherit_tags);
	win->w_tags_inherit->set_sensitive(part.base);
}

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

void PartEditor::run() {
		auto app = Gtk::Application::create("net.carrotIndustries.horizon.PartEditor", Gio::APPLICATION_NON_UNIQUE);

		win = PartEditorWindow::create();
		for(auto &it: win->attr_editors) {
			load_entry(it.second, &part, it.first);
		}
		update_tags_inherit();
		win->w_parametric->get_buffer()->set_text(serialize_parametric(part.parametric));
		win->w_parametric_from_base->set_sensitive(part.base);

		win->w_button_save->signal_clicked().connect(sigc::mem_fun(this, &PartEditor::save));

		{
			std::stringstream s;
			std::copy(part.tags.begin(), part.tags.end(), std::ostream_iterator<std::string>(s, " "));
			win->w_tags->set_text(s.str());
		}

		entity_button = Gtk::manage(new EntityButton(pool));
		entity_button->show();
		if(part.entity)
			entity_button->property_selected_uuid() = part.entity->uuid;
		win->w_entity_button_box->pack_start(*entity_button, true, true, 0);

		package_button = Gtk::manage(new PackageButton(pool));
		package_button->show();
		if(part.package)
			package_button->property_selected_uuid() = part.package->uuid;
		win->w_package_button_box->pack_start(*package_button, true, true, 0);

		part_button = Gtk::manage(new PartButton(pool));
		part_button->show();
		part_button->property_selected_uuid() = part.base?part.base->uuid:horizon::UUID();
		win->w_part_button_box->pack_start(*part_button, true, true, 0);

		pin_store = Gtk::ListStore::create(pin_list_columns);
		win->w_tv_pins->set_model(pin_store);

		win->w_tv_pins->append_column("Gate", pin_list_columns.gate_name);
		win->w_tv_pins->append_column("Pin", pin_list_columns.pin_name);
		{
			auto cr = Gtk::manage(new Gtk::CellRendererPixbuf());
			cr->property_icon_name() = "object-select-symbolic";
			cr->property_xalign() = 0;
			auto tvc = Gtk::manage(new Gtk::TreeViewColumn("Mapped", *cr));
			tvc->add_attribute(cr->property_visible(), pin_list_columns.mapped);
			win->w_tv_pins->append_column(*tvc);
		}

		win->w_tv_pins->get_column(0)->set_sort_column(pin_list_columns.gate_name);
		win->w_tv_pins->get_column(1)->set_sort_column(pin_list_columns.pin_name);

		pad_store = Gtk::ListStore::create(pad_list_columns);
		pad_store->set_sort_func(pad_list_columns.pad_name, [this](const Gtk::TreeModel::iterator &ia, const Gtk::TreeModel::iterator &ib) {
			Gtk::TreeModel::Row ra = *ia;
			Gtk::TreeModel::Row rb = *ib;
			Glib::ustring a = ra[pad_list_columns.pad_name];
			Glib::ustring b = rb[pad_list_columns.pad_name];
			auto ca = g_utf8_collate_key_for_filename(a.c_str(), -1);
			auto cb = g_utf8_collate_key_for_filename(b.c_str(), -1);
			auto r = strcmp(ca, cb);
			g_free(ca);
			g_free(cb);
			return r;
		});
		win->w_tv_pads->set_model(pad_store);
		win->w_tv_pads->append_column("Pad", pad_list_columns.pad_name);
		/*{
			auto cr = Gtk::manage(new Gtk::CellRendererText());
			win->w_tv_pads->append_column("Pad", *cr);
			win->w_tv_pads->get_column(0)->set_cell_data_func(*cr, [this](Gtk::CellRenderer *c, const Gtk::TreeModel::iterator &it){
				auto ct = dynamic_cast<Gtk::CellRendererText*>(c);
				ct->property_text() = part.package->pads.at(it[])
			});
		}*/
		win->w_tv_pads->append_column("Gate", pad_list_columns.gate_name);
		win->w_tv_pads->append_column("Pin", pad_list_columns.pin_name);
		win->w_tv_pads->get_column(0)->set_sort_column(pad_list_columns.pad_name);


		update_treeview();

		win->w_button_unmap->signal_clicked().connect([this] {
			auto sel = win->w_tv_pads->get_selection();
			for(auto &path: sel->get_selected_rows()) {
				auto it = pad_store->get_iter(path);
				Gtk::TreeModel::Row row = *it;
				row[pad_list_columns.gate_name] = "";
				row[pad_list_columns.pin_name] = "";
				row[pad_list_columns.pin_uuid] = horizon::UUID();
				row[pad_list_columns.gate_uuid] = horizon::UUID();
			}
			update_mapped();
		});
		win->w_button_map->signal_clicked().connect([this] {
			auto pin_sel = win->w_tv_pins->get_selection();
			auto it_pin = pin_sel->get_selected();
			if(it_pin) {
				Gtk::TreeModel::Row row_pin = *it_pin;
				std::cout << "map" << std::endl;
				auto sel = win->w_tv_pads->get_selection();
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
			}
		});

		entity_button->property_selected_uuid().signal_changed().connect([this]{
			horizon::UUID uu = entity_button->property_selected_uuid();
			if(uu != (part.entity?part.entity->uuid:horizon::UUID::random())) {
				part.entity = pool.get_entity(uu);
				part.pad_map.clear();
				update_treeview();
			}
		});
		package_button->property_selected_uuid().signal_changed().connect([this]{
			horizon::UUID uu = package_button->property_selected_uuid();
			if(uu != (part.package?part.package->uuid:horizon::UUID::random())) {
				part.package = pool.get_package(uu);
				part.pad_map.clear();
				update_treeview();
			}
		});
		part_button->property_selected_uuid().signal_changed().connect([this]{
			horizon::UUID uu = part_button->property_selected_uuid();
			if(uu == part.uuid) {
				part_button->property_selected_uuid() = horizon::UUID();
				return;
			}
			horizon::UUID base_uu;
			if(part.base) {
				base_uu = part.base->uuid;
			}
			if(uu != base_uu) {
				if(uu) {
					part.base = pool.get_part(uu);
					part.entity = part.base->entity;
					part.package = part.base->package;
					entity_button->property_selected_uuid() = part.entity->uuid;
					package_button->property_selected_uuid() = part.package->uuid;
					for(auto &it: win->attr_editors) {
						it.second->property_can_inherit() = true;
						load_entry_base(it.second, &part, it.first);
					}
					part.pad_map = part.base->pad_map;
				}
				else {
					part.base = nullptr;
					part.inherit_tags = false;
					for(auto &it: win->attr_editors) {
						it.second->property_inherit() = false;
						it.second->property_can_inherit() = false;
					}
					part.pad_map.clear();
				}
				update_tags_inherit();
				win->w_parametric_from_base->set_sensitive(part.base);
				package_button->set_sensitive(!part.base);
				entity_button->set_sensitive(!part.base);
				win->w_button_map->set_sensitive(!part.base);
				win->w_button_unmap->set_sensitive(!part.base);
				update_treeview();
				part.pad_map.clear();

			}
		});

		win->w_parametric_from_base->signal_clicked().connect([this] {
			if(part.base) {
				win->w_parametric->get_buffer()->set_text(serialize_parametric(part.base->parametric));
			}
		});

		package_button->set_sensitive(!part.base);
		entity_button->set_sensitive(!part.base);
		win->w_button_map->set_sensitive(!part.base);
		win->w_button_unmap->set_sensitive(!part.base);


		app->run(*win);

}

void PartEditor::save() {
	for(auto &it: win->attr_editors) {
		part.attributes[it.first] = it.second->get_as_pair();
	}
	//part.attributes[horizon::Part::Attribute::MPN] = {false, win->w_mpn->get_text()};
	//part.value_raw = win->w_value->get_text();
	//part.manufacturer_raw = win->w_manufacturer->get_text();
	{
		std::stringstream ss(win->w_tags->get_text());
		std::istream_iterator<std::string> begin(ss);
		std::istream_iterator<std::string> end;
		std::vector<std::string> tags(begin, end);
		part.tags.clear();
		part.tags.insert(tags.begin(), tags.end());
	}
	part.inherit_tags = part.base&&win->w_tags_inherit->get_active();

	part.parametric.clear();
	auto para = YAML::Load(win->w_parametric->get_buffer()->get_text());
	for(const auto &it: para) {
		part.parametric.emplace(it.first.as<std::string>(), it.second.as<std::string>());
	}

	part.pad_map.clear();
	for(const auto &it: pad_store->children()) {
		if(it[pad_list_columns.gate_uuid] != horizon::UUID()) {
			const horizon::Gate *gate = &part.entity->gates.at(it[pad_list_columns.gate_uuid]);
			const horizon::Pin *pin = &gate->unit->pins.at(it[pad_list_columns.pin_uuid]);
			part.pad_map.emplace(it[pad_list_columns.pad_uuid], horizon::Part::PadMapItem(gate, pin));
		}
	}
	horizon::save_json_to_file(filename, part.serialize());
}
