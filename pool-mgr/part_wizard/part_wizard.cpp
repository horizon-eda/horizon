#include "part_wizard.hpp"
#include "package.hpp"
#include "util.hpp"
#include "pad_editor.hpp"
#include "gate_editor.hpp"
#include "location_entry.hpp"

namespace horizon {
	LocationEntry *PartWizard::pack_location_entry(const Glib::RefPtr<Gtk::Builder>& x, const std::string &w, Gtk::Button **button_other) {
		auto en = Gtk::manage(new LocationEntry());
		if(button_other) {
			*button_other = Gtk::manage(new Gtk::Button());
			en->pack_start(**button_other, false, false);
			(*button_other)->show();
		}
		Gtk::Box *box;
		x->get_widget(w, box);
		box->pack_start(*en, true, true, 0);
		en->show();
		return en;
	}

	PartWizard::PartWizard(BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>& x, const Package *p, const std::string &bp, class Pool *po) :
		Gtk::Window(cobject), pkg(p), pool_base_path(bp), pool(po), part(UUID::random()), entity(UUID::random()) {
		x->get_widget("stack", stack);
		x->get_widget("button_back", button_back);
		x->get_widget("button_next", button_next);
		x->get_widget("button_finish", button_finish);

		x->get_widget("pads_lb", pads_lb);
		x->get_widget("link_pads", button_link_pads);
		x->get_widget("unlink_pads", button_unlink_pads);
		x->get_widget("import_pads", button_import_pads);
		x->get_widget("page_assign", page_assign);
		x->get_widget("page_edit", page_edit);
		x->get_widget("edit_left_box", edit_left_box);

		x->get_widget("entity_name", entity_name_entry);
		x->get_widget("entity_name_from_mpn", entity_name_from_mpn_button);
		x->get_widget("entity_prefix", entity_prefix_entry);
		x->get_widget("entity_tags", entity_tags_entry);

		x->get_widget("part_mpn", part_mpn_entry);
		x->get_widget("part_value", part_value_entry);
		x->get_widget("part_manufacturer", part_manufacturer_entry);
		x->get_widget("part_tags", part_tags_entry);

		part_location_entry = pack_location_entry(x, "part_location_box");
		part_location_entry->set_filename(Glib::build_filename(pool_base_path, "parts"));
		{
			Gtk::Button *from_part_button;
			entity_location_entry = pack_location_entry(x, "entity_location_box", &from_part_button);
			from_part_button->set_label("From part");
			from_part_button->signal_clicked().connect([this] {
				auto part_fn = Gio::File::create_for_path(part_location_entry->get_filename());
				auto part_base = Gio::File::create_for_path(Glib::build_filename(pool_base_path, "parts"));
				auto rel = part_base->get_relative_path(part_fn);
				entity_location_entry->set_filename(Glib::build_filename(pool_base_path, "entities", rel));
			});
			entity_location_entry->set_filename(Glib::build_filename(pool_base_path, "entities"));
		}

		entity_name_from_mpn_button->signal_clicked().connect([this]{
			entity_name_entry->set_text(part_mpn_entry->get_text());
		});

		entity_prefix_entry->set_text("U");

		sg_name = Gtk::SizeGroup::create(Gtk::SIZE_GROUP_HORIZONTAL);

		part.attributes[Part::Attribute::MANUFACTURER] = {false, pkg->manufacturer};

		gate_name_store = Gtk::ListStore::create(list_columns);

		{
			Gtk::TreeModel::Row row = *(gate_name_store->append());
			row[list_columns.name] = "Main";
		}

		create_pad_editors();

		pads_lb->set_sort_func([](Gtk::ListBoxRow *a, Gtk::ListBoxRow *b){
			auto na = dynamic_cast<PadEditor*>(a->get_child())->names.front();
			auto nb = dynamic_cast<PadEditor*>(b->get_child())->names.front();
			return strcmp_natural(na, nb);
		});

		part.package = pkg;
		part.entity = &entity;


		button_link_pads->signal_clicked().connect(sigc::mem_fun(this, &PartWizard::handle_link));
		button_unlink_pads->signal_clicked().connect(sigc::mem_fun(this, &PartWizard::handle_unlink));
		button_import_pads->signal_clicked().connect(sigc::mem_fun(this, &PartWizard::handle_import));
		button_next->signal_clicked().connect(sigc::mem_fun(this, &PartWizard::handle_next));
		button_back->signal_clicked().connect(sigc::mem_fun(this, &PartWizard::handle_back));
		button_finish->signal_clicked().connect(sigc::mem_fun(this, &PartWizard::handle_finish));

		signal_delete_event().connect([this](GdkEventAny *ev) {
			if(processes.size()){
				Gtk::MessageDialog md(*this,  "Can't close right now", false /* use_markup */, Gtk::MESSAGE_ERROR, Gtk::BUTTONS_OK);
				md.set_secondary_text("Close all running editors first");
				md.run();
				return true; //keep open
			}
			return false;
		});

		set_mode(Mode::ASSIGN);
	}

	void PartWizard::create_pad_editors() {
		for(auto &it: pkg->pads) {
			if(it.second.pool_padstack->type != Padstack::Type::MECHANICAL) {
				auto ed = PadEditor::create(&it.second, this);
				ed->pin_name_entry->signal_activate().connect([this, ed] {
					auto row = dynamic_cast<Gtk::ListBoxRow*>(ed->get_ancestor(GTK_TYPE_LIST_BOX_ROW));
					auto index = row->get_index();
					if(index >= 0) {
						if(auto nextrow = pads_lb->get_row_at_index(index+1)) {
							auto ed_next = dynamic_cast<PadEditor*>(nextrow->get_child());
							ed_next->pin_name_entry->grab_focus();
						}
					}
				});
				ed->show_all();
				pads_lb->append(*ed);
				ed->unreference();
			}
		}
	}

	void PartWizard::set_mode(PartWizard::Mode mo) {
		if(mo == Mode::ASSIGN && processes.size()>0)
			return;
		if(mo == Mode::ASSIGN) {
			stack->set_visible_child("assign");
		}
		else {
			prepare_edit();
			stack->set_visible_child("edit");
		}

		button_back->set_visible(mo == Mode::EDIT);
		button_finish->set_visible(mo == Mode::EDIT);
		button_next->set_visible(mo == Mode::ASSIGN);
		mode = mo;
	}

	void PartWizard::handle_next() {
		if(mode == Mode::ASSIGN) {
			set_mode(Mode::EDIT);
		}

	}

	void PartWizard::handle_back() {
		if(processes.size())
			return;
		set_mode(Mode::ASSIGN);
	}

	void PartWizard::handle_finish() {
		try {
			finish();
			has_finished = true;
			Gtk::Window::close();
		}
		catch (const std::exception& e) {
			Gtk::MessageDialog md(*this,  "Error Saving part", false /* use_markup */, Gtk::MESSAGE_ERROR, Gtk::BUTTONS_OK);
			md.set_secondary_text(e.what());
			md.run();
		}
		catch (const Gio::Error& e) {
			Gtk::MessageDialog md(*this,  "Error Saving part", false /* use_markup */, Gtk::MESSAGE_ERROR, Gtk::BUTTONS_OK);
			md.set_secondary_text(e.what());
			md.run();
		}
	}

	bool PartWizard::get_has_finished() const {
		return has_finished;
	}

	std::set<std::string> tags_from_string(const std::string &s) {
		std::stringstream ss(s);
		std::istream_iterator<std::string> begin(ss);
		std::istream_iterator<std::string> end;
		std::vector<std::string> tags(begin, end);
		std::set<std::string> tagss;
		tagss.insert(tags.begin(), tags.end());
		return tagss;
	}

	void PartWizard::finish() {
		entity.name = entity_name_entry->get_text();
		entity.prefix = entity_prefix_entry->get_text();
		entity.tags = tags_from_string(entity_tags_entry->get_text());

		part.attributes[Part::Attribute::MPN] = {false, part_mpn_entry->get_text()};
		part.attributes[Part::Attribute::VALUE] = {false, part_value_entry->get_text()};
		part.attributes[Part::Attribute::MANUFACTURER] = {false, part_manufacturer_entry->get_text()};
		part.tags = tags_from_string(part_tags_entry->get_text());

		entity.manufacturer = part.get_manufacturer();

		std::vector<std::string> filenames;
		filenames.push_back(entity_location_entry->get_filename());
		filenames.push_back(part_location_entry->get_filename());

		auto children = edit_left_box->get_children();
		for(auto ch: children) {
			if(auto ed = dynamic_cast<GateEditorWizard*>(ch)) {
				auto unit_filename = ed->unit_location_entry->get_filename();
				auto symbol_filename = ed->symbol_location_entry->get_filename();
				filenames.push_back(unit_filename);
				filenames.push_back(symbol_filename);

				auto unit = &units.at(ed->gate->name);
				assert(unit == ed->gate->unit);

				ed->gate->suffix = ed->suffix_entry->get_text();
				unit->name = ed->unit_name_entry->get_text();
				unit->manufacturer = part.get_manufacturer();
			}
		}

		for(const auto &it: filenames) {
			if(!endswith(it, ".json")) {
				throw std::runtime_error("Filename "+it+" doesn't end in .json");
			}
		}

		for(const auto &it: filenames) {
			auto dir = Glib::path_get_dirname(it);
			Gio::File::create_for_path(dir)->make_directory_with_parents();
		}

		for(auto ch: children) {
			if(auto ed = dynamic_cast<GateEditorWizard*>(ch)) {
				auto unit_filename = ed->unit_location_entry->get_filename();
				save_json_to_file(unit_filename, ed->gate->unit->serialize());

				auto symbol_filename_dest = ed->symbol_location_entry->get_filename();
				auto symbol_filename_src = pool->get_tmp_filename(ObjectType::SYMBOL, symbols.at(ed->gate->unit->uuid));
				auto fi_src = Gio::File::create_for_path(symbol_filename_src);
				auto fi_dest = Gio::File::create_for_path(symbol_filename_dest);
				fi_src->copy(fi_dest);
			}
		}
		save_json_to_file(entity_location_entry->get_filename(), entity.serialize());
		save_json_to_file(part_location_entry->get_filename(), part.serialize());
	}

	void PartWizard::handle_link() {
		auto rows = pads_lb->get_selected_rows();
		std::deque<PadEditor*> editors;

		for(auto row: rows) {
			auto ed = dynamic_cast<PadEditor*>(row->get_child());
			editors.push_back(ed);
		}
		link_pads(editors);
	}

	void PartWizard::link_pads(const std::deque<PadEditor*> &editors) {
		PadEditor *target = nullptr;
		if(editors.size() < 2)
			return;
		for(auto ed: editors) {
			if(ed->pads.size()>1) {
				target = ed;
				break;
			}
		}
		if(!target) {
			for(auto &ed: editors) {
				if(ed->pin_name_entry->get_text().size())
					target = ed;
			}
		}
		if(!target) {
			target = editors.front();
		}
		for(auto ed: editors) {
			if(ed != target) {
				target->pads.insert(ed->pads.begin(), ed->pads.end());
			}
		}
		target->update_names();
		for(auto ed: editors) {
			if(ed != target) {
				auto row = dynamic_cast<Gtk::ListBoxRow*>(ed->get_ancestor(GTK_TYPE_LIST_BOX_ROW));
				delete row;
			}
		}
		pads_lb->invalidate_sort();
	}

	void PartWizard::handle_unlink() {
		auto rows = pads_lb->get_selected_rows();
		for(auto row: rows) {
			auto ed = dynamic_cast<PadEditor*>(row->get_child());
			if(ed->pads.size()>1) {
				auto pad_keep = *ed->pads.begin();
				for(auto pad: ed->pads) {
					if(pad != pad_keep) {
						auto ed_new = PadEditor::create(pad, this);
						ed_new->show_all();
						pads_lb->append(*ed_new);
						ed_new->unreference();
						ed_new->pin_name_entry->set_text(ed->pin_name_entry->get_text());
						ed_new->dir_combo->set_active_id(ed->dir_combo->get_active_id());
						ed_new->combo_gate_entry->set_text(ed->combo_gate_entry->get_text());
						ed_new->update_names();
					}
				}

				ed->pads = {pad_keep};
				ed->update_names();
			}
		}
		pads_lb->invalidate_sort();
	}

	void PartWizard::handle_import() {
		GtkFileChooserNative *native = gtk_file_chooser_native_new ("Open",
			gobj(),
			GTK_FILE_CHOOSER_ACTION_OPEN,
			"_Save",
			"_Cancel");
		auto chooser = Glib::wrap(GTK_FILE_CHOOSER(native));
		auto filter= Gtk::FileFilter::create();
		filter->set_name("json documents");
		filter->add_pattern("*.json");
		chooser->add_filter(filter);

		if(gtk_native_dialog_run (GTK_NATIVE_DIALOG (native))==GTK_RESPONSE_ACCEPT) {
			auto filename = chooser->get_filename();
			try {
				json j;
				std::ifstream ifs(filename);
				if(!ifs.is_open()) {
					throw std::runtime_error("file "  +filename+ " not opened");
				}
				ifs>>j;
				ifs.close();
				import_pads(j);
			}
			catch (const std::exception& e) {
				Gtk::MessageDialog md(*this,  "Error importing", false /* use_markup */, Gtk::MESSAGE_ERROR, Gtk::BUTTONS_OK);
				md.set_secondary_text(e.what());
				md.run();
			}
			catch (...) {
				Gtk::MessageDialog md(*this,  "Error importing", false /* use_markup */, Gtk::MESSAGE_ERROR, Gtk::BUTTONS_OK);
				md.set_secondary_text("unknown error");
				md.run();
			}
		}

	}

	void PartWizard::import_pads(const json &j) {
		auto chs = pads_lb->get_children();
		for(auto ch: chs) {
			delete ch;
		}
		create_pad_editors();
		frozen = true;
		for(auto &ch: pads_lb->get_children()) {
			auto ed = dynamic_cast<PadEditor*>(dynamic_cast<Gtk::ListBoxRow*>(ch)->get_child());
			auto pad_name = (*ed->pads.begin())->name;
			if(j.count(pad_name)) {
				auto &k = j.at(pad_name);
				ed->pin_name_entry->set_text(k.value("pin", ""));
				ed->combo_gate_entry->set_text(k.value("gate", "Main"));
				if(k.count("alt")) {
					std::stringstream ss;
					for(auto &it: k.at("alt")) {
						ss << it.get<std::string>() << " ";
					}
					ed->pin_names_entry->set_text(ss.str());
				}
			}
		}
		autolink_pads();
		frozen = false;
		update_gate_names();
		update_pin_warnings();
	}

	void PartWizard::autolink_pads() {
		auto pin_names = get_pin_names();
		for(const auto &it: pin_names) {
			if(it.second.size() > 1) {
				std::deque<PadEditor*> pads(it.second.begin(), it.second.end());
				link_pads(pads);
			}
		}
	}

	void PartWizard::update_gate_names() {
		if(frozen)
			return;
		std::set<std::string> names;
		names.insert("Main");
		for(auto &ch: pads_lb->get_children()) {
			auto ed = dynamic_cast<PadEditor*>(dynamic_cast<Gtk::ListBoxRow*>(ch)->get_child());
			auto name = ed->get_gate_name();
			if(name.size()) {
				names.insert(name);
			}
		}
		std::vector<std::string> names_sorted(names.begin(), names.end());
		std::sort(names_sorted.begin(), names_sorted.end());
		gate_name_store->freeze_notify();
		gate_name_store->clear();
		for(const auto &name: names_sorted) {
			Gtk::TreeModel::Row row = *(gate_name_store->append());
			row[list_columns.name] = name;
		}
		gate_name_store->thaw_notify();
	}

	void PartWizard::prepare_edit() {
		std::set<Gate*> gates_avail;
		update_part();
		auto children = edit_left_box->get_children();
		for(auto ch: children) {
			if(auto ed = dynamic_cast<GateEditorWizard*>(ch)) {
				if(!entity.gates.count(ed->gate.uuid)) {
					delete ed;
				}
				else {
					std::cout << "found gate " << (std::string)ed->gate->uuid << std::endl;
					gates_avail.insert(ed->gate);
				}
			}
		}


		for(auto &it: entity.gates) {
			if(!gates_avail.count(&it.second)) {
				auto ed = GateEditorWizard::create(&it.second, this);
				edit_left_box->pack_start(*ed, false, false, 0);
				ed->show_all();
				ed->edit_symbol_button->signal_clicked().connect([this, ed] {
					auto symbol_uuid = symbols.at(ed->gate->unit->uuid);
					auto symbol_filename = pool->get_tmp_filename(ObjectType::SYMBOL, symbol_uuid);
					spawn(PoolManagerProcess::Type::IMP_SYMBOL, {symbol_filename});
				});

				ed->unreference();
			}
		}

	}

	std::map<std::pair<std::string, std::string>, std::set<class PadEditor*>> PartWizard::get_pin_names() {
		std::map<std::pair<std::string, std::string>, std::set<class PadEditor*>> pin_names;
		for(auto &ch: pads_lb->get_children()) {
			auto ed = dynamic_cast<PadEditor*>(dynamic_cast<Gtk::ListBoxRow*>(ch)->get_child());
			std::pair<std::string, std::string> key(ed->combo_gate_entry->get_text(), ed->pin_name_entry->get_text());
			if(key.second.size()) {
				pin_names[key];
				pin_names[key].insert(ed);
			}
		}
		return pin_names;
	}

	void PartWizard::update_pin_warnings() {
		if(frozen)
			return;
		auto pin_names = get_pin_names();
		bool has_warning = pin_names.size()==0;
		for(auto &it: pin_names) {
			std::string icon_name;
			if(it.second.size()>1) {
				icon_name = "dialog-warning-symbolic";
				has_warning = true;
			}
			for(auto ed: it.second) {
				ed->pin_name_entry->set_icon_from_icon_name(icon_name, Gtk::ENTRY_ICON_SECONDARY);
			}
		}
		button_next->set_sensitive(!has_warning);
	}

	void PartWizard::update_part() {
		std::cout << "upd part" << std::endl;
		std::set<UUID> pins_used;
		std::set<UUID> units_used;
		for(auto &ch: pads_lb->get_children()) {
			auto ed = dynamic_cast<PadEditor*>(dynamic_cast<Gtk::ListBoxRow*>(ch)->get_child());
			std::string pin_name = ed->pin_name_entry->get_text();
			std::string gate_name = ed->combo_gate_entry->get_text();
			if(pin_name.size()) {
				if(!units.count(gate_name)) {
					auto uu = UUID::random();
					units.emplace(gate_name, uu);
				}
				Unit *u = &units.at(gate_name);
				units_used.insert(u->uuid);
				Pin *pin = nullptr;
				{
					auto pi = std::find_if(u->pins.begin(), u->pins.end(), [pin_name](const auto &a){
						return a.second.primary_name == pin_name;
					});
					if(pi != u->pins.end()) {
						pin = &pi->second;
					}
				}
				if(!pin) {
					auto uu = UUID::random();
					pin = &u->pins.emplace(uu, uu).first->second;
				}
				pins_used.insert(pin->uuid);
				pin->primary_name = pin_name;
				pin->direction = static_cast<Pin::Direction>(std::stoi(ed->dir_combo->get_active_id()));
				{
					std::stringstream ss(ed->pin_names_entry->get_text());
					std::istream_iterator<std::string> begin(ss);
					std::istream_iterator<std::string> end;
					std::vector<std::string> tags(begin, end);
					pin->names = tags;
				}

				Gate *gate = nullptr;
				{
					auto gi = std::find_if(entity.gates.begin(), entity.gates.end(), [gate_name](const auto &a){
						return a.second.name == gate_name;
					});
					if(gi != entity.gates.end()) {
						gate = &gi->second;
					}
				}
				if(!gate) {
					auto uu = UUID::random();
					gate = &entity.gates.emplace(uu, uu).first->second;
				}
				gate->name = gate_name;
				gate->unit = &units.at(gate_name);

				for(auto pad: ed->pads) {
					part.pad_map.emplace(std::piecewise_construct, std::forward_as_tuple(pad->uuid), std::forward_as_tuple(gate, pin));
				}
			}
		}

		map_erase_if(entity.gates, [units_used](auto x){return units_used.count(x.second.unit->uuid)==0;});

		for (auto it = units.begin(); it != units.end();) {
			auto uu = it->second.uuid;
			if(units_used.count(uu)==0) {
				std::cout << "del sym" <<std::endl;

				{
					auto unit_filename = pool->get_tmp_filename(ObjectType::UNIT, uu);
					auto fi = Gio::File::create_for_path(unit_filename);
					fi->remove();
				}

				{
					auto sym_filename = pool->get_tmp_filename(ObjectType::SYMBOL, symbols.at(uu));
					auto fi = Gio::File::create_for_path(sym_filename);
					fi->remove();
					symbols.erase(uu);
				}
				units.erase(it++);

			}
			else {
				it++;
			}

		}
		for(auto &it: units) {
			map_erase_if(it.second.pins, [pins_used](auto x){return pins_used.count(x.second.uuid)==0;});
		}



		for(const auto &it: units) {
			if(!symbols.count(it.second.uuid)) {
				Symbol sym(UUID::random());
				sym.unit = &it.second;
				sym.name = "edit me";
				auto filename = pool->get_tmp_filename(ObjectType::SYMBOL, sym.uuid);
				save_json_to_file(filename, sym.serialize());
				symbols.emplace(sym.unit->uuid, sym.uuid);
			}
			auto filename = pool->get_tmp_filename(ObjectType::UNIT, it.second.uuid);
			save_json_to_file(filename, it.second.serialize());
		}
	}

	void PartWizard::spawn(PoolManagerProcess::Type type, const std::vector<std::string> &args) {
		if(processes.count(args.at(0)) == 0) { //need to launch imp
			std::vector<std::string> env = {"HORIZON_POOL="+pool_base_path};
			std::string filename = args.at(0);
			auto &proc = processes.emplace(std::piecewise_construct, std::forward_as_tuple(filename),
					std::forward_as_tuple(type, args, env, pool)).first->second;
			button_finish->set_sensitive(false);
			button_back->set_sensitive(false);
			proc.signal_exited().connect([filename, this](int status, bool need_update) {
				std::cout << "exit stat " << status << std::endl;
				/*if(status != 0) {
					view_project.info_bar_label->set_text("Editor for '"+filename+"' exited with status "+std::to_string(status));
					view_project.info_bar->show();

					//ugly workaround for making the info bar appear
					auto parent = dynamic_cast<Gtk::Box*>(view_project.info_bar->get_parent());
					parent->child_property_padding(*view_project.info_bar) = 1;
					parent->child_property_padding(*view_project.info_bar) = 0;
				}*/
				processes.erase(filename);
				button_finish->set_sensitive(processes.size()==0);
				button_back->set_sensitive(processes.size()==0);
			});
		}
		else { //present imp
			//auto &proc = processes.at(args.at(0));
			//auto pid = proc.proc->get_pid();
			//app->send_json(pid, {{"op", "present"}});
		}
	}


	PartWizard* PartWizard::create(const Package *p, const std::string &bp, class Pool *po) {
		PartWizard* w;
		Glib::RefPtr<Gtk::Builder> x = Gtk::Builder::create();
		x->add_from_resource("/net/carrotIndustries/horizon/pool-mgr/part_wizard/part_wizard.ui");
		x->get_widget_derived("part_wizard", w, p, bp, po);
		return w;
	}

	PartWizard::~PartWizard() {
		for(auto &it: units) {
			auto filename = pool->get_tmp_filename(ObjectType::UNIT, it.second.uuid);
			Gio::File::create_for_path(filename)->remove();
		}
		for(auto &it: symbols) {
			auto filename = pool->get_tmp_filename(ObjectType::SYMBOL, it.second);
			Gio::File::create_for_path(filename)->remove();
		}
	}

}
