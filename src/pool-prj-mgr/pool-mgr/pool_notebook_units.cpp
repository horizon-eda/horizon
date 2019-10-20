#include "pool_notebook.hpp"
#include "editors/editor_window.hpp"
#include "util/util.hpp"
#include "nlohmann/json.hpp"
#include "pool-prj-mgr/pool-prj-mgr-app_win.hpp"
#include "widgets/pool_browser_unit.hpp"
#include "pool_remote_box.hpp"
#include "widgets/unit_preview.hpp"
#include "widgets/unit_info_box.hpp"

namespace horizon {
void PoolNotebook::handle_create_unit()
{
    appwin->spawn(PoolProjectManagerProcess::Type::UNIT, {""});
}

void PoolNotebook::handle_edit_unit(const UUID &uu)
{
    if (!uu)
        return;
    UUID item_pool_uuid;
    auto path = pool.get_filename(ObjectType::UNIT, uu, &item_pool_uuid);
    appwin->spawn(PoolProjectManagerProcess::Type::UNIT, {path}, {}, pool_uuid && (item_pool_uuid != pool_uuid));
}

void PoolNotebook::handle_create_symbol_for_unit(const UUID &uu)
{
    if (!uu)
        return;
    auto top = dynamic_cast<Gtk::Window *>(get_ancestor(GTK_TYPE_WINDOW));

    GtkFileChooserNative *native =
            gtk_file_chooser_native_new("Save Symbol", top->gobj(), GTK_FILE_CHOOSER_ACTION_SAVE, "_Save", "_Cancel");
    auto chooser = Glib::wrap(GTK_FILE_CHOOSER(native));
    chooser->set_do_overwrite_confirmation(true);
    {
        auto unit_filename = pool.get_filename(ObjectType::UNIT, uu);
        auto unit_dirname_rel =
                Gio::File::create_for_path(Glib::build_filename(base_path, "units"))
                        ->get_relative_path(Gio::File::create_for_path(Glib::path_get_dirname(unit_filename)));

        chooser->set_current_folder(Glib::build_filename(base_path, "symbols", unit_dirname_rel));
        chooser->set_current_name(Glib::path_get_basename(unit_filename));
    }

    if (gtk_native_dialog_run(GTK_NATIVE_DIALOG(native)) == GTK_RESPONSE_ACCEPT) {
        std::string fn = EditorWindow::fix_filename(chooser->get_filename());
        Symbol sym(horizon::UUID::random());
        auto unit = pool.get_unit(uu);
        sym.name = unit->name;
        sym.unit = unit;
        save_json_to_file(fn, sym.serialize());
        appwin->spawn(PoolProjectManagerProcess::Type::IMP_SYMBOL, {fn});
    }
}

void PoolNotebook::handle_create_entity_for_unit(const UUID &uu)
{
    if (!uu)
        return;
    auto top = dynamic_cast<Gtk::Window *>(get_ancestor(GTK_TYPE_WINDOW));

    GtkFileChooserNative *native =
            gtk_file_chooser_native_new("Save Entity", top->gobj(), GTK_FILE_CHOOSER_ACTION_SAVE, "_Save", "_Cancel");
    auto chooser = Glib::wrap(GTK_FILE_CHOOSER(native));
    chooser->set_do_overwrite_confirmation(true);
    {
        auto unit_filename = pool.get_filename(ObjectType::UNIT, uu);
        auto unit_dirname_rel =
                Gio::File::create_for_path(Glib::build_filename(base_path, "units"))
                        ->get_relative_path(Gio::File::create_for_path(Glib::path_get_dirname(unit_filename)));

        chooser->set_current_folder(Glib::build_filename(base_path, "entities", unit_dirname_rel));
        chooser->set_current_name(Glib::path_get_basename(unit_filename));
    }

    if (gtk_native_dialog_run(GTK_NATIVE_DIALOG(native)) == GTK_RESPONSE_ACCEPT) {
        std::string fn = EditorWindow::fix_filename(chooser->get_filename());
        Entity entity(horizon::UUID::random());
        auto unit = pool.get_unit(uu);
        entity.name = unit->name;
        auto uu2 = UUID::random();
        auto gate = &entity.gates.emplace(uu2, uu2).first->second;
        gate->unit = unit;
        gate->name = "Main";

        save_json_to_file(fn, entity.serialize());
        appwin->spawn(PoolProjectManagerProcess::Type::ENTITY, {fn});
    }
}

void PoolNotebook::handle_duplicate_unit(const UUID &uu)
{
    if (!uu)
        return;
    show_duplicate_window(ObjectType::UNIT, uu);
}

void PoolNotebook::construct_units()
{
    auto br = Gtk::manage(new PoolBrowserUnit(&pool));
    br->set_show_path(true);
    br->signal_activated().connect([this, br] { handle_edit_unit(br->get_selected()); });

    br->show();
    browsers.emplace(ObjectType::UNIT, br);

    auto box = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_VERTICAL, 0));
    auto bbox = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL, 8));
    bbox->set_margin_bottom(8);
    bbox->set_margin_top(8);
    bbox->set_margin_start(8);
    bbox->set_margin_end(8);

    add_action_button("Create", bbox, sigc::mem_fun(*this, &PoolNotebook::handle_create_unit));
    add_action_button("Edit", bbox, br, sigc::mem_fun(*this, &PoolNotebook::handle_edit_unit));
    add_action_button("Duplicate", bbox, br, sigc::mem_fun(*this, &PoolNotebook::handle_duplicate_unit));
    add_action_button("Create Symbol", bbox, br, sigc::mem_fun(*this, &PoolNotebook::handle_create_symbol_for_unit));
    add_action_button("Create Entity", bbox, br, sigc::mem_fun(*this, &PoolNotebook::handle_create_entity_for_unit));
    if (remote_repo.size())
        add_action_button("Merge", bbox, br, [this](const UUID &uu) { remote_box->merge_item(ObjectType::UNIT, uu); });

    auto stack = Gtk::manage(new Gtk::Stack);
    add_preview_stack_switcher(bbox, stack);

    bbox->show_all();

    box->pack_start(*bbox, false, false, 0);

    auto sep = Gtk::manage(new Gtk::Separator(Gtk::ORIENTATION_HORIZONTAL));
    sep->show();
    box->pack_start(*sep, false, false, 0);
    box->pack_start(*br, true, true, 0);

    auto paned = Gtk::manage(new Gtk::Paned(Gtk::ORIENTATION_HORIZONTAL));
    paned->add1(*box);
    paned->child_property_shrink(*box) = false;

    auto preview = Gtk::manage(new UnitPreview(pool));
    preview->signal_goto().connect(sigc::mem_fun(*this, &PoolNotebook::go_to));

    auto info_box = UnitInfoBox::create(pool);
    info_box->signal_goto().connect(sigc::mem_fun(*this, &PoolNotebook::go_to));

    br->signal_selected().connect([this, br, preview, info_box] {
        auto sel = br->get_selected();
        if (!sel) {
            preview->load(nullptr);
            info_box->load(nullptr);
            return;
        }
        auto unit = pool.get_unit(sel);
        preview->load(unit);
        info_box->load(unit);
    });
    stack->add(*preview, "preview");
    stack->add(*info_box, "info");
    info_box->unreference();

    paned->add2(*stack);
    paned->show_all();

    stack->set_visible_child(*preview);

    append_page(*paned, "Units");
    br->search_once(); // we're the first page to be displayed
}

} // namespace horizon
