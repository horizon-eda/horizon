#include "pool_notebook.hpp"
#include "editors/editor_window.hpp"
#include "duplicate/duplicate_unit.hpp"
#include "util/util.hpp"
#include "nlohmann/json.hpp"
#include "pool-prj-mgr/pool-prj-mgr-app_win.hpp"
#include "widgets/pool_browser_padstack.hpp"
#include "widgets/padstack_preview.hpp"
#include "widgets/where_used_box.hpp"
#include "pool_remote_box.hpp"

namespace horizon {
void PoolNotebook::handle_edit_padstack(const UUID &uu)
{
    if (!uu)
        return;
    UUID item_pool_uuid;
    auto path = pool.get_filename(ObjectType::PADSTACK, uu, &item_pool_uuid);
    appwin->spawn(PoolProjectManagerProcess::Type::IMP_PADSTACK, {path}, {},
                  pool_uuid && (item_pool_uuid != pool_uuid));
}

void PoolNotebook::handle_create_padstack()
{
    auto top = dynamic_cast<Gtk::Window *>(get_ancestor(GTK_TYPE_WINDOW));

    GtkFileChooserNative *native =
            gtk_file_chooser_native_new("Save Padstack", top->gobj(), GTK_FILE_CHOOSER_ACTION_SAVE, "_Save", "_Cancel");
    auto chooser = Glib::wrap(GTK_FILE_CHOOSER(native));
    chooser->set_do_overwrite_confirmation(true);
    chooser->set_current_name("padstack.json");
    chooser->set_current_folder(Glib::build_filename(base_path, "padstacks"));

    if (gtk_native_dialog_run(GTK_NATIVE_DIALOG(native)) == GTK_RESPONSE_ACCEPT) {
        std::string fn = EditorWindow::fix_filename(chooser->get_filename());
        Padstack ps(horizon::UUID::random());
        save_json_to_file(fn, ps.serialize());
        appwin->spawn(PoolProjectManagerProcess::Type::IMP_PADSTACK, {fn});
    }
}

void PoolNotebook::handle_duplicate_padstack(const UUID &uu)
{
    if (!uu)
        return;
    auto top = dynamic_cast<Gtk::Window *>(get_ancestor(GTK_TYPE_WINDOW));

    GtkFileChooserNative *native =
            gtk_file_chooser_native_new("Save Padstack", top->gobj(), GTK_FILE_CHOOSER_ACTION_SAVE, "_Save", "_Cancel");
    auto chooser = Glib::wrap(GTK_FILE_CHOOSER(native));
    chooser->set_do_overwrite_confirmation(true);
    auto ps_filename = pool.get_filename(ObjectType::PADSTACK, uu);
    auto ps_basename = Glib::path_get_basename(ps_filename);
    auto ps_dirname = Glib::path_get_dirname(ps_filename);
    chooser->set_current_folder(ps_dirname);
    chooser->set_current_name(DuplicateUnitWidget::insert_filename(ps_basename, "-copy"));

    if (gtk_native_dialog_run(GTK_NATIVE_DIALOG(native)) == GTK_RESPONSE_ACCEPT) {
        std::string fn = EditorWindow::fix_filename(chooser->get_filename());
        Padstack ps(*pool.get_padstack(uu));
        ps.name += " (Copy)";
        ps.uuid = UUID::random();
        save_json_to_file(fn, ps.serialize());
        appwin->spawn(PoolProjectManagerProcess::Type::IMP_PADSTACK, {fn});
    }
}

void PoolNotebook::construct_padstacks()
{
    auto br = Gtk::manage(new PoolBrowserPadstack(&pool));
    br->set_show_path(true);
    br->set_include_padstack_type(Padstack::Type::VIA, true);
    br->set_include_padstack_type(Padstack::Type::MECHANICAL, true);
    br->set_include_padstack_type(Padstack::Type::HOLE, true);
    br->signal_activated().connect([this, br] { handle_edit_padstack(br->get_selected()); });

    br->show();
    browsers.emplace(ObjectType::PADSTACK, br);

    auto paned = Gtk::manage(new Gtk::Paned(Gtk::ORIENTATION_HORIZONTAL));

    auto box = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_VERTICAL, 0));
    auto bbox = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL, 8));
    bbox->set_margin_bottom(8);
    bbox->set_margin_top(8);
    bbox->set_margin_start(8);
    bbox->set_margin_end(8);

    add_action_button("Create", bbox, sigc::mem_fun(*this, &PoolNotebook::handle_create_padstack));
    add_action_button("Edit", bbox, br, sigc::mem_fun(*this, &PoolNotebook::handle_edit_padstack));
    add_action_button("Duplicate", bbox, br, sigc::mem_fun(*this, &PoolNotebook::handle_duplicate_padstack));
    if (remote_repo.size())
        add_action_button("Merge", bbox, br,
                          [this](const UUID &uu) { remote_box->merge_item(ObjectType::PADSTACK, uu); });

    auto stack = Gtk::manage(new Gtk::Stack);
    add_preview_stack_switcher(bbox, stack);

    bbox->show_all();

    box->pack_start(*bbox, false, false, 0);

    auto sep = Gtk::manage(new Gtk::Separator(Gtk::ORIENTATION_HORIZONTAL));
    sep->show();
    box->pack_start(*sep, false, false, 0);
    box->pack_start(*br, true, true, 0);
    box->show();

    paned->add1(*box);
    paned->child_property_shrink(*box) = false;


    auto preview = Gtk::manage(new PadstackPreview(pool));
    preview->signal_goto().connect(sigc::mem_fun(*this, &PoolNotebook::go_to));

    auto where_used = Gtk::manage(new WhereUsedBox(pool));
    where_used->property_margin() = 10;
    where_used->signal_goto().connect(sigc::mem_fun(*this, &PoolNotebook::go_to));

    stack->add(*preview, "preview");
    stack->add(*where_used, "info");

    paned->add2(*stack);
    paned->show_all();

    br->signal_selected().connect([br, preview, where_used] {
        auto sel = br->get_selected();
        preview->load(sel);
        where_used->load(ObjectType::PADSTACK, sel);
    });

    append_page(*paned, "Padstacks");
    install_search_once(paned, br);
}

} // namespace horizon
