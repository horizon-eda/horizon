#include "pool_notebook.hpp"
#include "util/util.hpp"
#include "nlohmann/json.hpp"
#include "pool-prj-mgr/pool-prj-mgr-app_win.hpp"
#include "widgets/pool_browser_frame.hpp"
#include "util/win32_undef.hpp"
#include "widgets/preview_canvas.hpp"

namespace horizon {
void PoolNotebook::handle_edit_frame(const UUID &uu)
{
    handle_edit_item(ObjectType::FRAME, uu);
}

void PoolNotebook::handle_create_frame()
{
    auto top = dynamic_cast<Gtk::Window *>(get_ancestor(GTK_TYPE_WINDOW));
    UUID unit_uuid;

    GtkFileChooserNative *native =
            gtk_file_chooser_native_new("Save Frame", top->gobj(), GTK_FILE_CHOOSER_ACTION_SAVE, "_Save", "_Cancel");
    auto chooser = Glib::wrap(GTK_FILE_CHOOSER(native));
    chooser->set_do_overwrite_confirmation(true);
    chooser->set_current_folder(Glib::build_filename(base_path, "frames"));

    if (gtk_native_dialog_run(GTK_NATIVE_DIALOG(native)) == GTK_RESPONSE_ACCEPT) {
        std::string fn = append_dot_json(chooser->get_filename());
        Frame fr(horizon::UUID::random());
        fr.name = "fixme";
        save_json_to_file(fn, fr.serialize());
        pool_update({fn});
        appwin.spawn(PoolProjectManagerProcess::Type::IMP_FRAME, {fn});
    }
}

void PoolNotebook::handle_duplicate_frame(const UUID &uu)
{
    handle_duplicate_item(ObjectType::FRAME, uu);
}

void PoolNotebook::construct_frames()
{
    auto br = Gtk::manage(new PoolBrowserFrame(pool));
    br->set_show_path(true);
    browsers.emplace(ObjectType::FRAME, br);
    br->show();
    auto paned = Gtk::manage(new Gtk::Paned(Gtk::ORIENTATION_HORIZONTAL));

    auto box = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_VERTICAL, 0));
    auto bbox = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL, 8));
    bbox->set_margin_bottom(8);
    bbox->set_margin_top(8);
    bbox->set_margin_start(8);
    bbox->set_margin_end(8);

    add_action_button("Create", bbox, sigc::mem_fun(*this, &PoolNotebook::handle_create_frame));
    add_action_button("Edit", bbox, br, sigc::mem_fun(*this, &PoolNotebook::handle_edit_frame));
    add_action_button("Duplicate", bbox, br, sigc::mem_fun(*this, &PoolNotebook::handle_duplicate_frame));
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


    auto canvas = Gtk::manage(new PreviewCanvas(pool, false));
    paned->add2(*canvas);
    paned->show_all();

    br->signal_selected().connect([br, canvas] {
        auto sel = br->get_selected();
        canvas->load(ObjectType::FRAME, sel);
    });

    br->signal_activated().connect([this, br] { handle_edit_frame(br->get_selected()); });

    append_page(*paned, "Frames");
    install_search_once(paned, br);
    create_paned_state_store(paned, "frames");
}
} // namespace horizon
