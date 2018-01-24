#include "pool_notebook.hpp"
#include "editor_window.hpp"
#include "duplicate/duplicate_unit.hpp"
#include "duplicate/duplicate_part.hpp"
#include "part_wizard/part_wizard.hpp"
#include "util/util.hpp"

namespace horizon {
void PoolNotebook::handle_edit_package(const UUID &uu)
{
    if (!uu)
        return;
    auto path = pool.get_filename(ObjectType::PACKAGE, uu);
    spawn(PoolManagerProcess::Type::IMP_PACKAGE, {path});
}

void PoolNotebook::handle_create_package()
{
    auto top = dynamic_cast<Gtk::Window *>(get_ancestor(GTK_TYPE_WINDOW));

    GtkFileChooserNative *native = gtk_file_chooser_native_new(
            "Save Package", top->gobj(), GTK_FILE_CHOOSER_ACTION_CREATE_FOLDER, "_Save", "_Cancel");
    auto chooser = Glib::wrap(GTK_FILE_CHOOSER(native));
    chooser->set_do_overwrite_confirmation(true);
    chooser->set_current_name("package");
    chooser->set_current_folder(Glib::build_filename(base_path, "packages"));

    while (true) {
        chooser->set_current_folder(Glib::build_filename(base_path, "packages"));
        if (gtk_native_dialog_run(GTK_NATIVE_DIALOG(native)) == GTK_RESPONSE_ACCEPT) {
            std::string fn = chooser->get_filename();
            std::cout << "pkg " << fn << std::endl;

            Glib::Dir dir(fn);
            int n = 0;
            for (const auto &it : dir) {
                (void)it;
                n++;
            }
            if (n > 0) {
                Gtk::MessageDialog md(*top, "Folder must be empty", false /* use_markup */, Gtk::MESSAGE_ERROR,
                                      Gtk::BUTTONS_OK);
                md.run();
                continue;
            }
            else {
                auto fi = Gio::File::create_for_path(Glib::build_filename(fn, "padstacks"));
                fi->make_directory_with_parents();
                Package pkg(horizon::UUID::random());
                auto pkg_filename = Glib::build_filename(fn, "package.json");
                save_json_to_file(pkg_filename, pkg.serialize());
                spawn(PoolManagerProcess::Type::IMP_PACKAGE, {pkg_filename});
            }
        }
        break;
    }
}

void PoolNotebook::handle_create_padstack_for_package(const UUID &uu)
{
    if (!uu)
        return;
    auto top = dynamic_cast<Gtk::Window *>(get_ancestor(GTK_TYPE_WINDOW));

    GtkFileChooserNative *native =
            gtk_file_chooser_native_new("Save Padstack", top->gobj(), GTK_FILE_CHOOSER_ACTION_SAVE, "_Save", "_Cancel");
    auto chooser = Glib::wrap(GTK_FILE_CHOOSER(native));
    chooser->set_do_overwrite_confirmation(true);
    chooser->set_current_name("pad.json");
    auto pkg_filename = pool.get_filename(ObjectType::PACKAGE, uu);
    auto pkg_dir = Glib::path_get_dirname(pkg_filename);
    chooser->set_current_folder(Glib::build_filename(pkg_dir, "padstacks"));

    if (gtk_native_dialog_run(GTK_NATIVE_DIALOG(native)) == GTK_RESPONSE_ACCEPT) {
        std::string fn = EditorWindow::fix_filename(chooser->get_filename());
        Padstack ps(horizon::UUID::random());
        ps.name = "Pad";
        save_json_to_file(fn, ps.serialize());
        spawn(PoolManagerProcess::Type::IMP_PADSTACK, {fn});
    }
}

void PoolNotebook::handle_duplicate_package(const UUID &uu)
{
    if (!uu)
        return;
    auto top = dynamic_cast<Gtk::Window *>(get_ancestor(GTK_TYPE_WINDOW));

    GtkFileChooserNative *native = gtk_file_chooser_native_new(
            "Save Package", top->gobj(), GTK_FILE_CHOOSER_ACTION_CREATE_FOLDER, "_Save", "_Cancel");
    auto chooser = Glib::wrap(GTK_FILE_CHOOSER(native));
    chooser->set_do_overwrite_confirmation(true);
    auto pkg_filename = Glib::path_get_dirname(pool.get_filename(ObjectType::PACKAGE, uu));
    auto pkg_basename = Glib::path_get_basename(pkg_filename);
    auto pkg_dirname = Glib::path_get_dirname(pkg_filename);
    chooser->set_current_folder(pkg_dirname);
    chooser->set_current_name(pkg_basename + "-copy");

    while (true) {
        chooser->set_current_folder(pkg_dirname);
        if (gtk_native_dialog_run(GTK_NATIVE_DIALOG(native)) == GTK_RESPONSE_ACCEPT) {
            std::string fn = chooser->get_filename();

            Glib::Dir dir(fn);
            int n = 0;
            for (const auto &it : dir) {
                (void)it;
                n++;
            }
            if (n > 0) {
                Gtk::MessageDialog md(*top, "Folder must be empty", false /* use_markup */, Gtk::MESSAGE_ERROR,
                                      Gtk::BUTTONS_OK);
                md.run();
                continue;
            }
            else {
                auto pkg = pool.get_package(uu);
                DuplicatePartWidget::duplicate_package(&pool, uu, fn, pkg->name + " (Copy)");
                std::string new_pkg_filename = Glib::build_filename(fn, "package.json");
                pool_update(
                        [this, new_pkg_filename] { spawn(PoolManagerProcess::Type::IMP_PACKAGE, {new_pkg_filename}); });
            }
        }
        break;
    }
}

void PoolNotebook::handle_part_wizard(const UUID &uu)
{
    if (!part_wizard) {
        auto pkg = pool.get_package(uu);
        part_wizard = PartWizard::create(pkg, base_path, &pool);
        part_wizard->present();
        part_wizard->signal_hide().connect([this] {
            if (part_wizard->get_has_finished()) {
                pool_update();
            }
            delete part_wizard;
            part_wizard = nullptr;
        });
    }
    else {
        part_wizard->present();
    }
}
} // namespace horizon
