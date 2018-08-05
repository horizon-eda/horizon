#include "pool_notebook.hpp"
#include "editors/editor_window.hpp"
#include "duplicate/duplicate_unit.hpp"
#include "util/util.hpp"
#include "dialogs/pool_browser_dialog.hpp"
#include "widgets/pool_browser_unit.hpp"
#include "nlohmann/json.hpp"
#include "pool-prj-mgr/pool-prj-mgr-app_win.hpp"

namespace horizon {
void PoolNotebook::handle_edit_frame(const UUID &uu)
{
    if (!uu)
        return;
    UUID item_pool_uuid;
    auto path = pool.get_filename(ObjectType::FRAME, uu, &item_pool_uuid);
    appwin->spawn(PoolProjectManagerProcess::Type::IMP_FRAME, {path}, {}, pool_uuid && (item_pool_uuid != pool_uuid));
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
        std::string fn = EditorWindow::fix_filename(chooser->get_filename());
        Frame fr(horizon::UUID::random());
        fr.name = "fixme";
        save_json_to_file(fn, fr.serialize());
        appwin->spawn(PoolProjectManagerProcess::Type::IMP_FRAME, {fn});
    }
}

void PoolNotebook::handle_duplicate_frame(const UUID &uu)
{
    if (!uu)
        return;
    auto top = dynamic_cast<Gtk::Window *>(get_ancestor(GTK_TYPE_WINDOW));

    GtkFileChooserNative *native =
            gtk_file_chooser_native_new("Save Frame", top->gobj(), GTK_FILE_CHOOSER_ACTION_SAVE, "_Save", "_Cancel");
    auto chooser = Glib::wrap(GTK_FILE_CHOOSER(native));
    chooser->set_do_overwrite_confirmation(true);
    auto fr_filename = pool.get_filename(ObjectType::FRAME, uu);
    auto fr_basename = Glib::path_get_basename(fr_filename);
    auto fr_dirname = Glib::path_get_dirname(fr_filename);
    chooser->set_current_folder(fr_dirname);
    chooser->set_current_name(DuplicateUnitWidget::insert_filename(fr_basename, "-copy"));

    if (gtk_native_dialog_run(GTK_NATIVE_DIALOG(native)) == GTK_RESPONSE_ACCEPT) {
        std::string fn = EditorWindow::fix_filename(chooser->get_filename());
        Frame fr(*pool.get_frame(uu));
        fr.name += " (Copy)";
        fr.uuid = UUID::random();
        save_json_to_file(fn, fr.serialize());
        appwin->spawn(PoolProjectManagerProcess::Type::IMP_FRAME, {fn});
    }
}
} // namespace horizon
