#include "kicad_symbol_import_wizard.hpp"
#include "util/util.hpp"
#include "util/str_util.hpp"
#include "widgets/pool_browser_package.hpp"
#include "widgets/preview_canvas.hpp"
#include "nlohmann/json.hpp"
#include "pool-prj-mgr/pool-prj-mgr-app_win.hpp"
#include "gate_editor.hpp"
#include "../part_wizard/location_entry.hpp"
#include "canvas/canvas_gl.hpp"
#include "pool-prj-mgr/pool-mgr/editors/editor_window.hpp"

namespace horizon {

KiCadSymbolImportWizard::KiCadSymbolImportWizard(BaseObjectType *cobject, const Glib::RefPtr<Gtk::Builder> &x,
                                                 const UUID &pkg_uuid, class Pool &po,
                                                 class PoolProjectManagerAppWindow *aw, const std::string &lib_filename)
    : Gtk::Window(cobject), pool(po), appwin(aw), state_store(this, "kicad-symbol-import-wizard")
{
    x->get_widget("header", header);
    x->get_widget("stack", stack);
    x->get_widget("button_next", button_next);
    x->get_widget("button_finish", button_finish);
    x->get_widget("button_skip", button_skip);
    x->get_widget("button_prev", button_prev);
    x->get_widget("tv_symbols", tv_symbols);
    x->get_widget("edit_gates_box", edit_gates_box);
    x->get_widget("part_box", part_box);
    x->get_widget("preview_part_sp", preview_part_sp);
    x->get_widget("merge_pins_cb", merge_pins_cb);
    x->get_widget("fp_info_label", fp_info_label);
    x->get_widget("fp_info_label_sym", fp_info_label_sym);
    x->get_widget("button_autofill", button_autofill);

    x->get_widget("button_part_edit", button_part_edit);
    button_part_edit->signal_clicked().connect(sigc::mem_fun(*this, &KiCadSymbolImportWizard::handle_edit_part));
    {
        Gtk::Button *button_entity_edit;
        x->get_widget("button_entity_edit", button_entity_edit);
        button_entity_edit->signal_clicked().connect(
                sigc::mem_fun(*this, &KiCadSymbolImportWizard::handle_edit_entity));
    }

    {
        Gtk::Paned *pane_package;
        x->get_widget("pane_package", pane_package);
        browser_package = Gtk::manage(new PoolBrowserPackage(&pool, true));
        browser_package->show();
        pane_package->add1(*browser_package);

        auto preview = Gtk::manage(new PreviewCanvas(pool, true));
        browser_package->signal_selected().connect([this, preview] {
            preview->load(ObjectType::PACKAGE, browser_package->get_selected());
            update_buttons();
        });
        browser_package->signal_activated().connect(sigc::mem_fun(*this, &KiCadSymbolImportWizard::handle_next));
        browser_package->go_to(pkg_uuid);

        preview->show();
        pane_package->add2(*preview);
    }

    {
        Gtk::Box *symbol_preview_box = nullptr;
        x->get_widget("symbol_preview_box", symbol_preview_box);
        symbol_preview = Gtk::manage(new PreviewCanvas(pool, false));
        symbol_preview->show();
        symbol_preview_box->pack_start(*symbol_preview, true, true, 0);
    }
    preview_part_sp->signal_value_changed().connect(
            sigc::mem_fun(*this, &KiCadSymbolImportWizard::update_symbol_preview_part));

    set_mode(Mode::SYMBOL);

    k_symbols = parse_kicad_library(lib_filename);
    symbols_store = Gtk::ListStore::create(list_columns);
    for (const auto &it : k_symbols) {
        auto row = *symbols_store->append();
        row[list_columns.name] = it.name;
        row[list_columns.sym] = &it;
    }
    tv_symbols->set_model(symbols_store);
    tv_symbols->append_column("Symbol", list_columns.name);
    tv_symbols->set_search_column(list_columns.name);
    symbols_store->set_sort_column(list_columns.name, Gtk::SORT_ASCENDING);
    symbols_store->set_sort_func(list_columns.name,
                                 [this](const Gtk::TreeModel::iterator &ia, const Gtk::TreeModel::iterator &ib) {
                                     Gtk::TreeModel::Row ra = *ia;
                                     Gtk::TreeModel::Row rb = *ib;
                                     Glib::ustring a = ra[list_columns.name];
                                     Glib::ustring b = rb[list_columns.name];
                                     return strcmp_natural(a, b);
                                 });
    tv_symbols->get_column(0)->set_sort_column(list_columns.name);
    tv_symbols->signal_row_activated().connect(
            [this](const Gtk::TreeModel::Path &pat, Gtk::TreeViewColumn *col) { select_symbol(); });
    tv_symbols->get_selection()->signal_changed().connect(
            sigc::mem_fun(*this, &KiCadSymbolImportWizard::update_symbol_preview));
    merge_pins_cb->signal_toggled().connect([this] {
        auto p = preview_part_sp->get_value_as_int();
        update_symbol_preview();
        preview_part_sp->set_value(p);
    });

    button_next->signal_clicked().connect(sigc::mem_fun(*this, &KiCadSymbolImportWizard::handle_next));
    button_skip->signal_clicked().connect(sigc::mem_fun(*this, &KiCadSymbolImportWizard::handle_skip));
    button_finish->signal_clicked().connect(sigc::mem_fun(*this, &KiCadSymbolImportWizard::handle_finish));
    button_prev->signal_clicked().connect(sigc::mem_fun(*this, &KiCadSymbolImportWizard::handle_prev));
    button_autofill->signal_clicked().connect(sigc::mem_fun(*this, &KiCadSymbolImportWizard::autofill));

    {
        Gtk::Button *from_entity_button;
        part_location_entry = pack_location_entry(x, "part_location_box", &from_entity_button);
        part_location_entry->set_filename(Glib::build_filename(pool.get_base_path(), "parts"));
        part_location_entry->signal_changed().connect(
                sigc::mem_fun(*this, &KiCadSymbolImportWizard::update_can_finish));
        from_entity_button->set_label("From entity");
        from_entity_button->signal_clicked().connect([this] {
            auto rel = get_rel_entity_filename();
            part_location_entry->set_filename(Glib::build_filename(pool.get_base_path(), "parts", rel));
        });
    }


    entity_location_entry = pack_location_entry(x, "entity_location_box");
    entity_location_entry->set_filename(Glib::build_filename(pool.get_base_path(), "entities"));
    entity_location_entry->signal_changed().connect(sigc::mem_fun(*this, &KiCadSymbolImportWizard::update_can_finish));

    update_can_finish();

    signal_delete_event().connect([this](GdkEventAny *ev) {
        if (processes.size()) {
            Gtk::MessageDialog md(*this, "Can't close right now", false /* use_markup */, Gtk::MESSAGE_ERROR,
                                  Gtk::BUTTONS_OK);
            md.set_secondary_text("Close all running editors first");
            md.run();
            return true; // keep open
        }

        if (files_saved.size())
            return false;

        if (mode == Mode::PACKAGE || mode == Mode::SYMBOL)
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

void KiCadSymbolImportWizard::update_symbol_preview()
{
    update_buttons();
    auto it = tv_symbols->get_selection()->get_selected();
    if (it) {
        Gtk::TreeModel::Row row = *it;
        const KiCadSymbol &sym = *row[list_columns.sym];
        KiCadSymbolImporter importer(sym, nullptr, merge_pins_cb->get_active());
        symbols_for_preview.clear();
        for (const auto &it_sym : importer.get_symbols()) {
            symbols_for_preview.emplace_back(it_sym);
        }
        preview_part_sp->set_value(0);
        preview_part_sp->set_range(0, symbols_for_preview.size() - 1);
        update_symbol_preview_part();
        fp_info_label_sym->set_text(get_fp_info(sym));
    }
}

void KiCadSymbolImportWizard::update_symbol_preview_part()
{
    size_t idx = preview_part_sp->get_value_as_int();
    if (idx < symbols_for_preview.size()) {
        auto &sym = symbols_for_preview.at(idx);
        symbol_preview->get_canvas().update(sym, Placement(), false);
        auto bb = pad_bbox(sym.get_bbox(true), 1_mm);
        symbol_preview->get_canvas().zoom_to_bbox(bb);
    }
    else {
        symbol_preview->get_canvas().clear();
    }
}

std::vector<std::string> KiCadSymbolImportWizard::get_filenames()
{
    std::vector<std::string> filenames;
    auto children = edit_gates_box->get_children();
    for (auto ch : children) {
        if (auto ed = dynamic_cast<GateEditorImportWizard *>(ch)) {
            auto unit_filename = ed->unit_location_entry->get_filename();
            auto symbol_filename = ed->symbol_location_entry->get_filename();
            filenames.push_back(unit_filename);
            filenames.push_back(symbol_filename);
        }
    }
    filenames.push_back(entity_location_entry->get_filename());
    filenames.push_back(part_location_entry->get_filename());
    return filenames;
}


std::vector<std::string> KiCadSymbolImportWizard::get_files_saved() const
{
    return files_saved;
}

void KiCadSymbolImportWizard::handle_finish()
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
        files_saved = filenames;
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

void KiCadSymbolImportWizard::finish()
{
    if (processes.size())
        throw std::runtime_error("open editors");

    std::vector<std::string> filenames;
    filenames.push_back(entity_location_entry->get_filename());
    filenames.push_back(part_location_entry->get_filename());

    auto children = edit_gates_box->get_children();
    for (auto ch : children) {
        if (auto ed = dynamic_cast<GateEditorImportWizard *>(ch)) {
            auto unit_filename = ed->unit_location_entry->get_filename();
            auto symbol_filename = ed->symbol_location_entry->get_filename();
            filenames.push_back(unit_filename);
            filenames.push_back(symbol_filename);
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
        if (auto ed = dynamic_cast<GateEditorImportWizard *>(ch)) {
            {
                auto src = pool.get_tmp_filename(ObjectType::SYMBOL, symbols.at(ed->unit_uu));
                auto dest = ed->symbol_location_entry->get_filename();
                Gio::File::create_for_path(src)->copy(Gio::File::create_for_path(dest), Gio::FILE_COPY_OVERWRITE);
            }
            {
                auto src = pool.get_tmp_filename(ObjectType::UNIT, ed->unit_uu);
                auto dest = ed->unit_location_entry->get_filename();
                Gio::File::create_for_path(src)->copy(Gio::File::create_for_path(dest), Gio::FILE_COPY_OVERWRITE);
            }
        }
    }
    {
        auto src = pool.get_tmp_filename(ObjectType::ENTITY, entity_uuid);
        auto dest = entity_location_entry->get_filename();
        Gio::File::create_for_path(src)->copy(Gio::File::create_for_path(dest), Gio::FILE_COPY_OVERWRITE);
    }
    if (part_uuid) {
        auto src = pool.get_tmp_filename(ObjectType::PART, part_uuid);
        auto dest = part_location_entry->get_filename();
        Gio::File::create_for_path(src)->copy(Gio::File::create_for_path(dest), Gio::FILE_COPY_OVERWRITE);
    }
}

void KiCadSymbolImportWizard::handle_next()
{
    if (mode == Mode::SYMBOL) {
        select_symbol();
    }
    else if (mode == Mode::PACKAGE) {
        auto pkg = pool.get_package(browser_package->get_selected());
        import(pkg);
    }
}

void KiCadSymbolImportWizard::handle_prev()
{
    set_mode(Mode::SYMBOL);
}

void KiCadSymbolImportWizard::handle_skip()
{
    import(nullptr);
}

void KiCadSymbolImportWizard::set_mode(KiCadSymbolImportWizard::Mode mo)
{
    std::string title = "Import KiCad Symbol";
    if (mo == Mode::SYMBOL) {
        stack->set_visible_child("symbol");
        header->set_subtitle("Select symbol");
    }
    else if (mo == Mode::EDIT) {
        stack->set_visible_child("edit");
        header->set_subtitle("");
        title += " " + k_sym->name;
    }
    else if (mo == Mode::PACKAGE) {
        stack->set_visible_child("package");
        header->set_subtitle("Select package");
        title += " " + k_sym->name;
    }
    header->set_title(title);
    button_finish->set_visible(mo == Mode::EDIT);
    button_prev->set_visible(mo == Mode::PACKAGE);
    button_next->set_visible(mo != Mode::EDIT);
    button_skip->set_visible(mo == Mode::PACKAGE);
    mode = mo;
    update_buttons();
}

void KiCadSymbolImportWizard::import(const Package *pkg)
{
    set_mode(Mode::EDIT);

    KiCadSymbolImporter importer(*k_sym, pkg, merge_pins_cb->get_active());
    auto &units = importer.get_units();
    auto &syms = importer.get_symbols();

    auto &entity = importer.get_entity();
    entity_uuid = entity.uuid;
    if (auto part = importer.get_part()) {
        part_uuid = part->uuid;
        auto filename = pool.get_tmp_filename(ObjectType::PART, part_uuid);
        save_json_to_file(filename, part->serialize());
    }
    button_part_edit->set_sensitive(part_uuid);

    {
        auto filename = pool.get_tmp_filename(ObjectType::ENTITY, entity_uuid);
        save_json_to_file(filename, entity.serialize());
    }
    for (const auto &it : units) {
        auto filename = pool.get_tmp_filename(ObjectType::UNIT, it.uuid);
        save_json_to_file(filename, it.serialize());
    }
    for (const auto &it : syms) {
        auto filename = pool.get_tmp_filename(ObjectType::SYMBOL, it.uuid);
        save_json_to_file(filename, it.serialize());
        symbols.emplace(it.unit->uuid, it.uuid);
    }
    {
        std::vector<const Gate *> gates_sorted;
        std::transform(entity.gates.begin(), entity.gates.end(), std::back_inserter(gates_sorted),
                       [](auto &x) { return &x.second; });
        std::sort(gates_sorted.begin(), gates_sorted.end(),
                  [](const auto a, const auto b) { return strcmp_natural(a->name, b->name) < 0; });
        for (auto gate : gates_sorted) {
            auto ed = GateEditorImportWizard::create(gate->uuid, gate->unit->uuid, *this);
            edit_gates_box->pack_start(*ed, false, false, 0);
            ed->show_all();
        }
    }
    part_box->set_visible(part_uuid);
    entity_location_entry->set_filename(Glib::build_filename(pool.get_base_path(), "entities", k_sym->name + ".json"));
    update_can_finish();
}

void KiCadSymbolImportWizard::update_buttons()
{
    if (mode == Mode::SYMBOL) {
        button_next->set_sensitive(tv_symbols->get_selection()->count_selected_rows());
    }
    else if (mode == Mode::PACKAGE) {
        button_next->set_sensitive(browser_package->get_selected());
    }
}

std::string KiCadSymbolImportWizard::get_fp_info(const KiCadSymbol &sym)
{
    std::string t = std::to_string(sym.get_n_pins()) + " Pins ";
    t += "Footprint: " + sym.footprint;
    if (sym.fplist.size()) {
        t += " (";
        for (const auto &fp : sym.fplist) {
            t += fp + ", ";
        }
        t.pop_back();
        t.pop_back();
        t += ")";
    }
    return t;
}

void KiCadSymbolImportWizard::select_symbol()
{
    auto it = tv_symbols->get_selection()->get_selected();
    if (it) {
        Gtk::TreeModel::Row row = *it;
        k_sym = row[list_columns.sym];
        fp_info_label->set_text(get_fp_info(*k_sym));
        browser_package->set_pads_filter(k_sym->get_n_pins());
        set_mode(Mode::PACKAGE);
    }
}

void KiCadSymbolImportWizard::handle_edit_entity()
{
    std::string entity_filename = pool.get_tmp_filename(ObjectType::ENTITY, entity_uuid);
    auto proc = appwin->spawn(PoolProjectManagerProcess::Type::ENTITY, {entity_filename});
    processes.emplace(entity_filename, proc);
    proc->signal_exited().connect([this, entity_filename](int status, bool modified) {
        processes.erase(entity_filename);
        update_can_finish();
    });
    proc->win->signal_goto().connect([this](ObjectType type, UUID uu) {
        if (type == ObjectType::UNIT) {
            for (auto ch : edit_gates_box->get_children()) {
                if (auto c = dynamic_cast<GateEditorImportWizard *>(ch)) {
                    if (c->unit_uu == uu) {
                        c->handle_edit_unit();
                    }
                }
            }
        }
    });
    update_can_finish();
}

void KiCadSymbolImportWizard::handle_edit_part()
{
    std::string part_filename = pool.get_tmp_filename(ObjectType::PART, part_uuid);
    auto proc = appwin->spawn(PoolProjectManagerProcess::Type::PART, {part_filename});
    processes.emplace(part_filename, proc);
    proc->signal_exited().connect([this, part_filename](int status, bool modified) {
        processes.erase(part_filename);
        update_can_finish();
    });
    proc->win->signal_goto().connect([this](ObjectType type, UUID uu) {
        if (type == ObjectType::ENTITY) { // only one entity
            handle_edit_entity();
        }
    });
    update_can_finish();
}

std::string KiCadSymbolImportWizard::get_rel_entity_filename()
{
    auto entity_fn = Gio::File::create_for_path(entity_location_entry->get_filename());
    auto base = Gio::File::create_for_path(Glib::build_filename(pool.get_base_path(), "entities"));
    auto rel = base->get_relative_path(entity_fn);
    return rel;
}

void KiCadSymbolImportWizard::autofill()
{
    auto rel = get_rel_entity_filename();
    part_location_entry->set_filename(Glib::build_filename(pool.get_base_path(), "parts", rel));
    const auto &entity = pool.get_entity(entity_uuid);
    auto children = edit_gates_box->get_children();
    for (auto ch : children) {
        if (auto ed = dynamic_cast<GateEditorImportWizard *>(ch)) {
            std::string suffix = entity->gates.at(ed->gate_uu).suffix;
            trim(suffix);
            ed->unit_location_entry->set_filename(
                    Glib::build_filename(pool.get_base_path(), "units", ed->get_suffixed_filename_from_entity()));
            ed->symbol_location_entry->set_filename(
                    Glib::build_filename(pool.get_base_path(), "symbols", ed->get_suffixed_filename_from_entity()));
        }
    }
}

LocationEntry *KiCadSymbolImportWizard::pack_location_entry(const Glib::RefPtr<Gtk::Builder> &x, const std::string &w,
                                                            Gtk::Button **button_other)
{
    auto en = Gtk::manage(new LocationEntry(pool.get_base_path()));
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

void KiCadSymbolImportWizard::update_can_finish()
{
    bool editors_open = processes.size() > 0;
    bool valid = true;


    valid = part_location_entry->check_ends_json() && valid;
    valid = entity_location_entry->check_ends_json() && valid;

    std::set<std::string> symbol_filenames;
    std::set<std::string> unit_filenames;

    auto children = edit_gates_box->get_children();
    for (auto ch : children) {
        if (auto ed = dynamic_cast<GateEditorImportWizard *>(ch)) {
            ed->unit_location_entry->set_warning("");
            ed->symbol_location_entry->set_warning("");

            valid = ed->unit_location_entry->check_ends_json() && valid;
            valid = ed->symbol_location_entry->check_ends_json() && valid;

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
        }
    }
    button_finish->set_sensitive(!editors_open && valid);
}

KiCadSymbolImportWizard *KiCadSymbolImportWizard::create(const UUID &pkg_uuid, class Pool &po,
                                                         class PoolProjectManagerAppWindow *aw,
                                                         const std::string &lib_filename)
{
    KiCadSymbolImportWizard *w;
    Glib::RefPtr<Gtk::Builder> x = Gtk::Builder::create();
    x->add_from_resource(
            "/org/horizon-eda/horizon/pool-prj-mgr/pool-mgr/kicad_symbol_import_wizard/kicad_symbol_import_wizard.ui");
    x->get_widget_derived("window", w, pkg_uuid, po, aw, lib_filename);
    return w;
}

KiCadSymbolImportWizard::~KiCadSymbolImportWizard()
{
    for (auto &it : symbols) {
        auto unit_filename = pool.get_tmp_filename(ObjectType::UNIT, it.first);
        auto sym_filename = pool.get_tmp_filename(ObjectType::SYMBOL, it.second);
        Gio::File::create_for_path(unit_filename)->remove();
        Gio::File::create_for_path(sym_filename)->remove();
    }
    if (entity_uuid) {
        auto filename = pool.get_tmp_filename(ObjectType::ENTITY, entity_uuid);
        Gio::File::create_for_path(filename)->remove();
    }
    if (part_uuid) {
        auto filename = pool.get_tmp_filename(ObjectType::PART, part_uuid);
        Gio::File::create_for_path(filename)->remove();
    }
}

} // namespace horizon
