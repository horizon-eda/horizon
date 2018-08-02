#include "part_wizard.hpp"
#include "pool/package.hpp"
#include "util/util.hpp"
#include "pad_editor.hpp"
#include "gate_editor.hpp"
#include "location_entry.hpp"
#include "util/str_util.hpp"
#include "util/gtk_util.hpp"
#include "util/pool_completion.hpp"
#include "util/csv.hpp"
#include "nlohmann/json.hpp"
#include "pool-prj-mgr/pool-prj-mgr-app_win.hpp"

namespace horizon {
LocationEntry *PartWizard::pack_location_entry(const Glib::RefPtr<Gtk::Builder> &x, const std::string &w,
                                               Gtk::Button **button_other)
{
    auto en = Gtk::manage(new LocationEntry());
    if (button_other) {
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

PartWizard::PartWizard(BaseObjectType *cobject, const Glib::RefPtr<Gtk::Builder> &x, const Package *p,
                       const std::string &bp, class Pool *po, PoolProjectManagerAppWindow *aw)
    : Gtk::Window(cobject), pkg(p), pool_base_path(bp), pool(po), part(UUID::random()), entity(UUID::random()),
      appwin(aw), state_store(this, "part-wizard")
{
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
    x->get_widget("part_description", part_description_entry);
    x->get_widget("part_datasheet", part_datasheet_entry);
    x->get_widget("part_tags", part_tags_entry);
    x->get_widget("part_autofill", part_autofill_button);
    x->get_widget("steps_grid", steps_grid);

    part_manufacturer_entry->set_completion(create_pool_manufacturer_completion(pool));

    part_mpn_entry->signal_changed().connect(sigc::mem_fun(this, &PartWizard::update_can_finish));
    entity_name_entry->signal_changed().connect(sigc::mem_fun(this, &PartWizard::update_can_finish));
    entity_prefix_entry->signal_changed().connect(sigc::mem_fun(this, &PartWizard::update_can_finish));

    part_autofill_button->signal_clicked().connect(sigc::mem_fun(this, &PartWizard::autofill));

    part_location_entry = pack_location_entry(x, "part_location_box");
    part_location_entry->set_filename(Glib::build_filename(pool_base_path, "parts"));
    part_location_entry->signal_changed().connect(sigc::mem_fun(this, &PartWizard::update_can_finish));
    {
        Gtk::Button *from_part_button;
        entity_location_entry = pack_location_entry(x, "entity_location_box", &from_part_button);
        from_part_button->set_label("From part");
        from_part_button->signal_clicked().connect([this] {
            auto rel = get_rel_part_filename();
            entity_location_entry->set_filename(Glib::build_filename(pool_base_path, "entities", rel));
        });
        entity_location_entry->set_filename(Glib::build_filename(pool_base_path, "entities"));
        entity_location_entry->signal_changed().connect(sigc::mem_fun(this, &PartWizard::update_can_finish));
    }

    entity_name_from_mpn_button->signal_clicked().connect(
            [this] { entity_name_entry->set_text(part_mpn_entry->get_text()); });

    entity_prefix_entry->set_text("U");

    sg_name = Gtk::SizeGroup::create(Gtk::SIZE_GROUP_HORIZONTAL);

    part.attributes[Part::Attribute::MANUFACTURER] = {false, pkg->manufacturer};

    gate_name_store = Gtk::ListStore::create(list_columns);

    {
        Gtk::TreeModel::Row row = *(gate_name_store->append());
        row[list_columns.name] = "Main";
    }

    create_pad_editors();

    pads_lb->set_sort_func([](Gtk::ListBoxRow *a, Gtk::ListBoxRow *b) {
        auto na = dynamic_cast<PadEditor *>(a->get_child())->names.front();
        auto nb = dynamic_cast<PadEditor *>(b->get_child())->names.front();
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

    set_mode(Mode::ASSIGN);

    signal_delete_event().connect([this](GdkEventAny *ev) {
        if (processes.size()) {
            Gtk::MessageDialog md(*this, "Can't close right now", false /* use_markup */, Gtk::MESSAGE_ERROR,
                                  Gtk::BUTTONS_OK);
            md.set_secondary_text("Close all running editors first");
            md.run();
            return true; // keep open
        }
        if (has_finished)
            return false;

        Gtk::MessageDialog md(*this, "Really close?", false /* use_markup */, Gtk::MESSAGE_QUESTION, Gtk::BUTTONS_NONE);
        md.set_secondary_text(
                "By closing the part wizard, all changes to the new part will "
                "be lost");
        md.add_button("Close and discard changes", 1);
        md.add_button("Keep open", Gtk::RESPONSE_CANCEL);
        switch (md.run()) {
        case 1:
            return false; // close

        default:
            return true; // keep window open
        }
    });
}

void PartWizard::create_pad_editors()
{
    for (auto &it : pkg->pads) {
        if (it.second.pool_padstack->type != Padstack::Type::MECHANICAL) {
            auto ed = PadEditor::create(&it.second, this);
            ed->pin_name_entry->signal_activate().connect([this, ed] {
                auto row = dynamic_cast<Gtk::ListBoxRow *>(ed->get_ancestor(GTK_TYPE_LIST_BOX_ROW));
                auto index = row->get_index();
                if (index >= 0) {
                    if (auto nextrow = pads_lb->get_row_at_index(index + 1)) {
                        auto ed_next = dynamic_cast<PadEditor *>(nextrow->get_child());
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

void PartWizard::set_mode(PartWizard::Mode mo)
{
    if (mo == Mode::ASSIGN && processes.size() > 0)
        return;
    if (mo == Mode::ASSIGN) {
        stack->set_visible_child("assign");
    }
    else {
        prepare_edit();
        stack->set_visible_child("edit");
        update_can_finish();
    }

    button_back->set_visible(mo == Mode::EDIT);
    button_finish->set_visible(mo == Mode::EDIT);
    button_next->set_visible(mo == Mode::ASSIGN);
    mode = mo;
}

void PartWizard::handle_next()
{
    if (mode == Mode::ASSIGN) {
        set_mode(Mode::EDIT);
    }
}

void PartWizard::handle_back()
{
    if (processes.size())
        return;
    set_mode(Mode::ASSIGN);
}

std::vector<std::string> PartWizard::get_filenames()
{
    std::vector<std::string> filenames;
    filenames.push_back(entity_location_entry->get_filename());
    filenames.push_back(part_location_entry->get_filename());
    auto children = edit_left_box->get_children();
    for (auto ch : children) {
        if (auto ed = dynamic_cast<GateEditorWizard *>(ch)) {
            auto unit_filename = ed->unit_location_entry->get_filename();
            auto symbol_filename = ed->symbol_location_entry->get_filename();
            filenames.push_back(unit_filename);
            filenames.push_back(symbol_filename);
        }
    }

    return filenames;
}

void PartWizard::handle_finish()
{
    auto filenames = get_filenames();
    std::vector<std::string> filenames_existing;
    for (const auto &filename : filenames) {
        if (Glib::file_test(filename, Glib::FILE_TEST_EXISTS)) {
            filenames_existing.push_back(filename);
        }
    }
    if (filenames_existing.size() > 0) {
        Gtk::MessageDialog md(*this, "Overwrite?", false /* use_markup */, Gtk::MESSAGE_QUESTION, Gtk::BUTTONS_NONE);
        std::string sec = "Creating this part will overwrite these files:\n";
        for (const auto &fn : filenames_existing) {
            sec += fn + "\n";
        }
        md.set_secondary_text(sec);
        md.add_button("Overwrite", 1);
        md.add_button("Cancel", Gtk::RESPONSE_CANCEL);
        switch (md.run()) {
        case 1:
            // nop, go ahead
            break;
        default:
            return;
        }
    }


    try {
        finish();
        has_finished = true;
        Gtk::Window::close();
    }
    catch (const std::exception &e) {
        Gtk::MessageDialog md(*this, "Error Saving part", false /* use_markup */, Gtk::MESSAGE_ERROR, Gtk::BUTTONS_OK);
        md.set_secondary_text(e.what());
        md.run();
    }
    catch (const Gio::Error &e) {
        Gtk::MessageDialog md(*this, "Error Saving part", false /* use_markup */, Gtk::MESSAGE_ERROR, Gtk::BUTTONS_OK);
        md.set_secondary_text(e.what());
        md.run();
    }
}

bool PartWizard::get_has_finished() const
{
    return has_finished;
}

std::set<std::string> tags_from_string(const std::string &s)
{
    std::stringstream ss(s);
    std::istream_iterator<std::string> begin(ss);
    std::istream_iterator<std::string> end;
    std::vector<std::string> tags(begin, end);
    std::set<std::string> tagss;
    tagss.insert(tags.begin(), tags.end());
    return tagss;
}

std::string PartWizard::get_rel_part_filename()
{
    auto part_fn = Gio::File::create_for_path(part_location_entry->get_filename());
    auto part_base = Gio::File::create_for_path(Glib::build_filename(pool_base_path, "parts"));
    auto rel = part_base->get_relative_path(part_fn);
    return rel;
}

void PartWizard::finish()
{
    entity.name = entity_name_entry->get_text();
    entity.prefix = entity_prefix_entry->get_text();
    entity.tags = tags_from_string(entity_tags_entry->get_text());

    part.attributes[Part::Attribute::MPN] = {false, part_mpn_entry->get_text()};
    part.attributes[Part::Attribute::VALUE] = {false, part_value_entry->get_text()};
    part.attributes[Part::Attribute::MANUFACTURER] = {false, part_manufacturer_entry->get_text()};
    part.attributes[Part::Attribute::DATASHEET] = {false, part_datasheet_entry->get_text()};
    part.attributes[Part::Attribute::DESCRIPTION] = {false, part_description_entry->get_text()};
    part.tags = tags_from_string(part_tags_entry->get_text());

    entity.manufacturer = part.get_manufacturer();

    std::vector<std::string> filenames;
    filenames.push_back(entity_location_entry->get_filename());
    filenames.push_back(part_location_entry->get_filename());

    auto children = edit_left_box->get_children();
    for (auto ch : children) {
        if (auto ed = dynamic_cast<GateEditorWizard *>(ch)) {
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

    for (const auto &it : filenames) {
        if (!endswith(it, ".json")) {
            throw std::runtime_error("Filename " + it + " doesn't end in .json");
        }
    }

    for (const auto &it : filenames) {
        auto dir = Glib::path_get_dirname(it);
        if (!Glib::file_test(dir, Glib::FILE_TEST_IS_DIR)) {
            Gio::File::create_for_path(dir)->make_directory_with_parents();
        }
    }

    for (auto ch : children) {
        if (auto ed = dynamic_cast<GateEditorWizard *>(ch)) {
            auto unit_filename = ed->unit_location_entry->get_filename();
            save_json_to_file(unit_filename, ed->gate->unit->serialize());

            auto symbol_filename_dest = ed->symbol_location_entry->get_filename();
            auto symbol_filename_src = pool->get_tmp_filename(ObjectType::SYMBOL, symbols.at(ed->gate->unit->uuid));
            auto fi_src = Gio::File::create_for_path(symbol_filename_src);
            auto fi_dest = Gio::File::create_for_path(symbol_filename_dest);
            fi_src->copy(fi_dest, Gio::FILE_COPY_OVERWRITE);
        }
    }
    save_json_to_file(entity_location_entry->get_filename(), entity.serialize());
    save_json_to_file(part_location_entry->get_filename(), part.serialize());
}

void PartWizard::handle_link()
{
    auto rows = pads_lb->get_selected_rows();
    std::deque<PadEditor *> editors;

    for (auto row : rows) {
        auto ed = dynamic_cast<PadEditor *>(row->get_child());
        editors.push_back(ed);
    }
    link_pads(editors);
}

void PartWizard::link_pads(const std::deque<PadEditor *> &editors)
{
    PadEditor *target = nullptr;
    if (editors.size() < 2)
        return;
    for (auto ed : editors) {
        if (ed->pads.size() > 1) {
            target = ed;
            break;
        }
    }
    if (!target) {
        for (auto &ed : editors) {
            if (ed->pin_name_entry->get_text().size())
                target = ed;
        }
    }
    if (!target) {
        target = editors.front();
    }
    for (auto ed : editors) {
        if (ed != target) {
            target->pads.insert(ed->pads.begin(), ed->pads.end());
        }
    }
    target->update_names();
    for (auto ed : editors) {
        if (ed != target) {
            auto row = dynamic_cast<Gtk::ListBoxRow *>(ed->get_ancestor(GTK_TYPE_LIST_BOX_ROW));
            delete row;
        }
    }
    pads_lb->invalidate_sort();
}

void PartWizard::handle_unlink()
{
    auto rows = pads_lb->get_selected_rows();
    for (auto row : rows) {
        auto ed = dynamic_cast<PadEditor *>(row->get_child());
        if (ed->pads.size() > 1) {
            auto pad_keep = *ed->pads.begin();
            for (auto pad : ed->pads) {
                if (pad != pad_keep) {
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

void pads_from_csv(CSV::csv &csv, json &j)
{
    for (auto &line : csv) {
        /* We need at least a non-zero pad name. */
        if (line.size() < 1) {
            continue;
        }
        std::string name = line[0];
        trim(name);
        if (name.empty()) {
            continue;
        }
        j[name] = json_from_fields(line, {"", "pin", "", "gate"});
    }
}

void PartWizard::handle_import()
{
    GtkFileChooserNative *native =
            gtk_file_chooser_native_new("Open", gobj(), GTK_FILE_CHOOSER_ACTION_OPEN, "_Save", "_Cancel");
    auto chooser = Glib::wrap(GTK_FILE_CHOOSER(native));
    auto filter = Gtk::FileFilter::create();
    filter->set_name("CSV documents");
    filter->add_pattern("*.csv");
    chooser->add_filter(filter);
    filter = Gtk::FileFilter::create();
    filter->set_name("json documents");
    filter->add_pattern("*.json");
    chooser->add_filter(filter);

    if (gtk_native_dialog_run(GTK_NATIVE_DIALOG(native)) == GTK_RESPONSE_ACCEPT) {
        auto filename = chooser->get_filename();
        try {
            json j;
            std::ifstream ifs(filename);
            if (!ifs.is_open()) {
                throw std::runtime_error("file " + filename + " not opened");
            }
            if (endswith(filename, ".json")) {
                ifs >> j;
                ifs.close();
            } else {
                CSV::csv csv;
                ifs >> csv;
                ifs.close();
                pads_from_csv(csv, j);
            }
            import_pads(j);
        }
        catch (const std::exception &e) {
            Gtk::MessageDialog md(*this, "Error importing", false /* use_markup */, Gtk::MESSAGE_ERROR,
                                  Gtk::BUTTONS_OK);
            md.set_secondary_text(e.what());
            md.run();
        }
        catch (...) {
            Gtk::MessageDialog md(*this, "Error importing", false /* use_markup */, Gtk::MESSAGE_ERROR,
                                  Gtk::BUTTONS_OK);
            md.set_secondary_text("unknown error");
            md.run();
        }
    }
}

void PartWizard::import_pads(const json &j)
{
    auto chs = pads_lb->get_children();
    for (auto ch : chs) {
        delete ch;
    }
    create_pad_editors();
    frozen = true;
    for (auto &ch : pads_lb->get_children()) {
        auto ed = dynamic_cast<PadEditor *>(dynamic_cast<Gtk::ListBoxRow *>(ch)->get_child());
        auto pad_name = (*ed->pads.begin())->name;
        if (j.count(pad_name)) {
            auto &k = j.at(pad_name);
            std::string pin_name = k.value("pin", "");
            std::string gate_name = k.value("gate", "Main");
            trim(pin_name);
            trim(gate_name);
            ed->pin_name_entry->set_text(pin_name);
            ed->combo_gate_entry->set_text(gate_name);
            if (k.count("alt")) {
                std::stringstream ss;
                for (auto &it : k.at("alt")) {
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

void PartWizard::autolink_pads()
{
    auto pin_names = get_pin_names();
    for (const auto &it : pin_names) {
        if (it.second.size() > 1) {
            std::deque<PadEditor *> pads(it.second.begin(), it.second.end());
            link_pads(pads);
        }
    }
}

void PartWizard::update_gate_names()
{
    if (frozen)
        return;
    std::set<std::string> names;
    names.insert("Main");
    for (auto &ch : pads_lb->get_children()) {
        auto ed = dynamic_cast<PadEditor *>(dynamic_cast<Gtk::ListBoxRow *>(ch)->get_child());
        auto name = ed->get_gate_name();
        if (name.size()) {
            names.insert(name);
        }
    }
    std::vector<std::string> names_sorted(names.begin(), names.end());
    std::sort(names_sorted.begin(), names_sorted.end());
    gate_name_store->freeze_notify();
    gate_name_store->clear();
    for (const auto &name : names_sorted) {
        Gtk::TreeModel::Row row = *(gate_name_store->append());
        row[list_columns.name] = name;
    }
    gate_name_store->thaw_notify();
}

void PartWizard::prepare_edit()
{
    std::set<Gate *> gates_avail;
    update_part();
    auto children = edit_left_box->get_children();
    for (auto ch : children) {
        if (auto ed = dynamic_cast<GateEditorWizard *>(ch)) {
            if (!entity.gates.count(ed->gate.uuid)) {
                delete ed;
            }
            else {
                std::cout << "found gate " << (std::string)ed->gate->uuid << std::endl;
                gates_avail.insert(ed->gate);
            }
        }
    }


    for (auto &it : entity.gates) {
        if (!gates_avail.count(&it.second)) {
            auto ed = GateEditorWizard::create(&it.second, this);
            edit_left_box->pack_start(*ed, false, false, 0);
            ed->show_all();
            ed->edit_symbol_button->signal_clicked().connect([this, ed] {
                auto symbol_uuid = symbols.at(ed->gate->unit->uuid);
                auto symbol_filename = pool->get_tmp_filename(ObjectType::SYMBOL, symbol_uuid);
                auto proc = appwin->spawn(PoolProjectManagerProcess::Type::IMP_SYMBOL, {symbol_filename});
                processes.emplace(symbol_filename, proc);
                proc->signal_exited().connect([this, symbol_filename](int status, bool modified) {
                    processes.erase(symbol_filename);
                    update_can_finish();
                });
                update_can_finish();
            });

            ed->unreference();
        }
    }
}

std::map<std::pair<std::string, std::string>, std::set<class PadEditor *>> PartWizard::get_pin_names()
{
    std::map<std::pair<std::string, std::string>, std::set<class PadEditor *>> pin_names;
    for (auto &ch : pads_lb->get_children()) {
        auto ed = dynamic_cast<PadEditor *>(dynamic_cast<Gtk::ListBoxRow *>(ch)->get_child());
        std::pair<std::string, std::string> key(ed->combo_gate_entry->get_text(), ed->pin_name_entry->get_text());
        if (key.second.size()) {
            pin_names[key];
            pin_names[key].insert(ed);
        }
    }
    return pin_names;
}

void PartWizard::update_pin_warnings()
{
    if (frozen)
        return;
    auto pin_names = get_pin_names();
    bool has_warning = pin_names.size() == 0;
    for (auto &it : pin_names) {
        std::string icon_name;
        if (it.second.size() > 1) {
            icon_name = "dialog-warning-symbolic";
            has_warning = true;
        }
        for (auto ed : it.second) {
            ed->pin_name_entry->set_icon_from_icon_name(icon_name, Gtk::ENTRY_ICON_SECONDARY);
        }
    }
    button_next->set_sensitive(!has_warning);
}

void PartWizard::update_part()
{
    std::cout << "upd part" << std::endl;
    std::set<UUID> pins_used;
    std::set<UUID> units_used;
    for (auto &ch : pads_lb->get_children()) {
        auto ed = dynamic_cast<PadEditor *>(dynamic_cast<Gtk::ListBoxRow *>(ch)->get_child());
        std::string pin_name = ed->pin_name_entry->get_text();
        std::string gate_name = ed->combo_gate_entry->get_text();
        if (pin_name.size()) {
            if (!units.count(gate_name)) {
                auto uu = UUID::random();
                units.emplace(gate_name, uu);
            }
            Unit *u = &units.at(gate_name);
            units_used.insert(u->uuid);
            Pin *pin = nullptr;
            {
                auto pi = std::find_if(u->pins.begin(), u->pins.end(),
                                       [pin_name](const auto &a) { return a.second.primary_name == pin_name; });
                if (pi != u->pins.end()) {
                    pin = &pi->second;
                }
            }
            if (!pin) {
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
                auto gi = std::find_if(entity.gates.begin(), entity.gates.end(),
                                       [gate_name](const auto &a) { return a.second.name == gate_name; });
                if (gi != entity.gates.end()) {
                    gate = &gi->second;
                }
            }
            if (!gate) {
                auto uu = UUID::random();
                gate = &entity.gates.emplace(uu, uu).first->second;
            }
            gate->name = gate_name;
            gate->unit = &units.at(gate_name);

            for (auto pad : ed->pads) {
                part.pad_map.emplace(std::piecewise_construct, std::forward_as_tuple(pad->uuid),
                                     std::forward_as_tuple(gate, pin));
            }
        }
    }

    map_erase_if(entity.gates, [units_used](auto x) { return units_used.count(x.second.unit->uuid) == 0; });

    for (auto it = units.begin(); it != units.end();) {
        auto uu = it->second.uuid;
        if (units_used.count(uu) == 0) {
            std::cout << "del sym" << std::endl;

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
    for (auto &it : units) {
        map_erase_if(it.second.pins, [pins_used](auto x) { return pins_used.count(x.second.uuid) == 0; });
    }


    for (const auto &it : units) {
        if (!symbols.count(it.second.uuid)) {
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

void PartWizard::autofill()
{
    entity_name_entry->set_text(part_mpn_entry->get_text());
    auto rel = get_rel_part_filename();
    entity_location_entry->set_filename(Glib::build_filename(pool_base_path, "entities", rel));
    auto children = edit_left_box->get_children();
    for (auto ch : children) {
        if (auto ed = dynamic_cast<GateEditorWizard *>(ch)) {
            std::string suffix = ed->suffix_entry->get_text();
            trim(suffix);
            if (suffix.size()) {
                ed->unit_name_entry->set_text(part_mpn_entry->get_text() + " " + suffix);
            }
            else {
                ed->unit_name_entry->set_text(part_mpn_entry->get_text());
            }
            ed->unit_location_entry->set_filename(
                    Glib::build_filename(pool_base_path, "units", ed->get_suffixed_filename_from_part()));
            ed->symbol_location_entry->set_filename(
                    Glib::build_filename(pool_base_path, "symbols", ed->get_suffixed_filename_from_part()));
        }
    }
}

void PartWizard::update_can_finish()
{
    bool editors_open = processes.size() > 0;
    button_back->set_sensitive(!editors_open);
    valid = true;

    auto check_entry_not_empty = [this](Gtk::Entry *e, const std::string &msg, bool *v = nullptr) {
        std::string t = e->get_text();
        trim(t);
        if (!t.size()) {
            entry_set_warning(e, msg);
            valid = false;
        }
        else {
            entry_set_warning(e, "");
        }
        if (v)
            *v = t.size();
    };
    auto check_location_ends_json = [this](LocationEntry *e, bool *v = nullptr) {
        std::string t = e->get_filename();
        if (!endswith(t, ".json")) {
            e->set_warning("Filename has to end in .json");
            valid = false;
            if (v)
                *v = false;
        }
        else {
            e->set_warning("");
            if (v)
                *v = true;
        }
    };

    check_entry_not_empty(part_mpn_entry, "MPN is empty", &mpn_valid);
    check_entry_not_empty(entity_name_entry, "Entity name is empty");
    check_entry_not_empty(entity_prefix_entry, "Entity prefix is empty");
    check_location_ends_json(part_location_entry, &part_filename_valid);

    std::set<std::string> symbol_filenames;
    std::set<std::string> unit_filenames;
    std::set<std::string> suffixes;
    std::set<std::string> unit_names;

    gates_valid = true;
    int n_gates = entity.gates.size();

    auto children = edit_left_box->get_children();
    for (auto ch : children) {
        if (auto ed = dynamic_cast<GateEditorWizard *>(ch)) {
            ed->unit_location_entry->set_warning("");
            ed->symbol_location_entry->set_warning("");
            entry_set_warning(ed->suffix_entry, "");
            entry_set_warning(ed->unit_name_entry, "");

            check_location_ends_json(ed->unit_location_entry);
            check_location_ends_json(ed->symbol_location_entry);

            check_entry_not_empty(ed->unit_name_entry, "Unit name is empty");
            std::string unit_filename = ed->unit_location_entry->get_filename();
            trim(unit_filename);

            if (!unit_filenames.insert(unit_filename).second) {
                ed->unit_location_entry->set_warning("Duplicate unit filename");
                valid = false;
            }

            std::string symbol_filename = ed->symbol_location_entry->get_filename();
            trim(symbol_filename);

            if (!symbol_filenames.insert(symbol_filename).second) {
                ed->symbol_location_entry->set_warning("Duplicate symbol filename");
                valid = false;
            }

            std::string suffix = ed->suffix_entry->get_text();
            trim(suffix);
            if (!suffixes.insert(suffix).second) {
                entry_set_warning(ed->suffix_entry, "Duplicate unit suffix");
                valid = false;
                gates_valid = false;
            }
            if (suffix.size() == 0 && (n_gates > 1)) {
                entry_set_warning(ed->suffix_entry, "Unit suffix is empty");
                valid = false;
                gates_valid = false;
            }

            std::string unit_name = ed->unit_name_entry->get_text();
            trim(unit_name);
            if (!unit_names.insert(unit_name).second) {
                entry_set_warning(ed->unit_name_entry, "Duplicate unit name");
                valid = false;
            }
        }
    }
    update_steps();
    button_finish->set_sensitive(!editors_open && valid);
}

void PartWizard::update_steps()
{
    auto chs = steps_grid->get_children();
    for (auto ch : chs) {
        delete ch;
    }
    int top = 0;
    auto add_step = [this, &top](const std::string &t, int st) {
        auto la = Gtk::manage(new Gtk::Label(t));
        la->set_halign(Gtk::ALIGN_START);
        steps_grid->attach(*la, 1, top, 1, 1);

        auto im = Gtk::manage(new Gtk::Image());
        if (st == 1) {
            im->set_from_icon_name("pan-end-symbolic", Gtk::ICON_SIZE_BUTTON);
        }
        else if (st == 2) {
            im->set_from_icon_name("object-select-symbolic", Gtk::ICON_SIZE_BUTTON);
        }
        steps_grid->attach(*im, 0, top, 1, 1);

        top++;
    };

    int progress = 0;

    auto compare_progress = [&progress](int x) {
        if (x == progress)
            return 1;
        else if (x < progress)
            return 2;
        return 0;
    };


    if (entity.gates.size() == 1) {
        if (mpn_valid) {
            progress = 1;
            if (part_filename_valid) {
                progress = 2;
                if (valid) {
                    progress = 3;
                }
            }
        }
        add_step("Enter MPN (and Manufacturer)", compare_progress(0));
        add_step("Enter part filename", compare_progress(1));
        add_step("Press Autofill", compare_progress(2));
    }
    else {
        if (mpn_valid) {
            progress = 1;
            if (part_filename_valid) {
                progress = 2;
                if (gates_valid) {
                    progress = 3;
                    if (valid) {
                        progress = 4;
                    }
                }
            }
        }
        add_step("Enter MPN (and Manufacturer)", compare_progress(0));
        add_step("Enter part filename", compare_progress(1));
        add_step("Enter Gate suffixes", compare_progress(2));
        add_step("Press Autofill", compare_progress(3));
    }


    steps_grid->show_all();
}

PartWizard *PartWizard::create(const Package *p, const std::string &bp, class Pool *po,
                               class PoolProjectManagerAppWindow *aw)
{
    PartWizard *w;
    Glib::RefPtr<Gtk::Builder> x = Gtk::Builder::create();
    x->add_from_resource(
            "/net/carrotIndustries/horizon/pool-prj-mgr/pool-mgr/part_wizard/"
            "part_wizard.ui");
    x->get_widget_derived("part_wizard", w, p, bp, po, aw);
    return w;
}

PartWizard::~PartWizard()
{
    for (auto &it : units) {
        auto filename = pool->get_tmp_filename(ObjectType::UNIT, it.second.uuid);
        Gio::File::create_for_path(filename)->remove();
    }
    for (auto &it : symbols) {
        auto filename = pool->get_tmp_filename(ObjectType::SYMBOL, it.second);
        Gio::File::create_for_path(filename)->remove();
    }
}
} // namespace horizon
