#include "pool_notebook.hpp"
#include "util/util.hpp"
#include "nlohmann/json.hpp"
#include "pool-prj-mgr/pool-prj-mgr-app_win.hpp"
#include "widgets/pool_browser_decal.hpp"
#include "util/win32_undef.hpp"
#include "widgets/preview_canvas.hpp"
#include "util/gtk_util.hpp"

namespace horizon {
void PoolNotebook::handle_edit_decal(const UUID &uu)
{
    handle_edit_item(ObjectType::DECAL, uu);
}

void PoolNotebook::handle_create_decal()
{
    auto top = dynamic_cast<Gtk::Window *>(get_ancestor(GTK_TYPE_WINDOW));
    UUID unit_uuid;

    GtkFileChooserNative *native =
            gtk_file_chooser_native_new("Save Decal", top->gobj(), GTK_FILE_CHOOSER_ACTION_SAVE, "_Save", "_Cancel");
    auto chooser = Glib::wrap(GTK_FILE_CHOOSER(native));
    chooser->set_do_overwrite_confirmation(true);
    chooser->set_current_folder(Glib::build_filename(base_path, "decals"));

    if (native_dialog_run_and_set_parent_insensitive(GTK_NATIVE_DIALOG(native)) == GTK_RESPONSE_ACCEPT) {
        std::string fn = append_dot_json(chooser->get_filename());
        Decal dec(horizon::UUID::random());
        save_json_to_file(fn, dec.serialize());
        pool_update({fn});
        appwin.spawn(PoolProjectManagerProcess::Type::IMP_DECAL, {fn});
    }
}

void PoolNotebook::handle_duplicate_decal(const UUID &uu)
{
    handle_duplicate_item(ObjectType::DECAL, uu);
}

void PoolNotebook::construct_decals()
{
    auto br = Gtk::manage(new PoolBrowserDecal(pool));
    br->set_show_path(true);
    browsers.emplace(ObjectType::DECAL, br);
    br->show();
    auto paned = Gtk::manage(new Gtk::Paned(Gtk::ORIENTATION_HORIZONTAL));

    auto box = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_VERTICAL, 0));
    auto bbox = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL, 8));
    bbox->set_margin_bottom(8);
    bbox->set_margin_top(8);
    bbox->set_margin_start(8);
    bbox->set_margin_end(8);

    add_action_button("Create", bbox, sigc::mem_fun(*this, &PoolNotebook::handle_create_decal));
    add_action_button("Edit", bbox, br, sigc::mem_fun(*this, &PoolNotebook::handle_edit_decal));
    add_action_button("Duplicate", bbox, br, sigc::mem_fun(*this, &PoolNotebook::handle_duplicate_decal));
    add_merge_button(bbox, br);
    bbox->show_all();

    box->pack_start(*bbox, false, false, 0);

    auto sep = Gtk::manage(new Gtk::Separator(Gtk::ORIENTATION_HORIZONTAL));
    sep->show();
    box->pack_start(*sep, false, false, 0);
    box->pack_start(*br, true, true, 0);
    box->show();

    paned->add1(*box);
    paned->child_property_shrink(*box) = false;


    auto canvas = Gtk::manage(new PreviewCanvas(pool, true));
    paned->add2(*canvas);
    paned->show_all();

    br->signal_selected().connect([br, canvas] {
        auto sel = br->get_selected();
        canvas->load(ObjectType::DECAL, sel);
    });

    br->signal_activated().connect([this, br] { handle_edit_decal(br->get_selected()); });

    append_page(*paned, "Decals");
    install_search_once(paned, br);
    create_paned_state_store(paned, "decals");
}
} // namespace horizon
