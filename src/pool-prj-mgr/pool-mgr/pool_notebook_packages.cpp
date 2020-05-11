#include "pool_notebook.hpp"
#include "editors/editor_window.hpp"
#include "duplicate/duplicate_unit.hpp"
#include "duplicate/duplicate_part.hpp"
#include "util/util.hpp"
#include "nlohmann/json.hpp"
#include "pool-prj-mgr/pool-prj-mgr-app_win.hpp"
#include "widgets/pool_browser_package.hpp"
#include "pool_remote_box.hpp"
#include "widgets/package_info_box.hpp"

#ifdef G_OS_WIN32
#undef ERROR
#endif
#include "widgets/preview_canvas.hpp"

namespace horizon {
void PoolNotebook::handle_edit_package(const UUID &uu)
{
    if (!uu)
        return;
    UUID item_pool_uuid;
    auto path = pool.get_filename(ObjectType::PACKAGE, uu, &item_pool_uuid);
    appwin->spawn(PoolProjectManagerProcess::Type::IMP_PACKAGE, {path}, {}, pool_uuid && (item_pool_uuid != pool_uuid));
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
                appwin->spawn(PoolProjectManagerProcess::Type::IMP_PACKAGE, {pkg_filename});
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
        appwin->spawn(PoolProjectManagerProcess::Type::IMP_PADSTACK, {fn});
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
                std::vector<std::string> filenames;
                DuplicatePartWidget::duplicate_package(&pool, uu, fn, Glib::path_get_basename(fn), &filenames);
                std::string new_pkg_filename = Glib::build_filename(fn, "package.json");
                pool_update(
                        [this, new_pkg_filename] {
                            appwin->spawn(PoolProjectManagerProcess::Type::IMP_PACKAGE, {new_pkg_filename});
                        },
                        filenames);
            }
        }
        break;
    }
}

void PoolNotebook::construct_packages()
{
    auto br = Gtk::manage(new PoolBrowserPackage(&pool));
    br->set_show_path(true);
    br->signal_activated().connect([this, br] { handle_edit_package(br->get_selected()); });

    br->show();
    browsers.emplace(ObjectType::PACKAGE, br);

    auto paned = Gtk::manage(new Gtk::Paned(Gtk::ORIENTATION_HORIZONTAL));


    auto box = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_VERTICAL, 0));
    auto bbox = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL, 8));
    bbox->set_margin_bottom(8);
    bbox->set_margin_top(8);
    bbox->set_margin_start(8);
    bbox->set_margin_end(8);

    add_action_button("Create", bbox, sigc::mem_fun(*this, &PoolNotebook::handle_create_package));
    add_action_button("Edit", bbox, br, sigc::mem_fun(*this, &PoolNotebook::handle_edit_package));
    add_action_button("Duplicate", bbox, br, sigc::mem_fun(*this, &PoolNotebook::handle_duplicate_package));
    add_action_button("Create Padstack", bbox, br,
                      sigc::mem_fun(*this, &PoolNotebook::handle_create_padstack_for_package));
    if (remote_repo.size()) {
        add_action_button("Merge", bbox, br,
                          [this](const UUID &uu) { remote_box->merge_item(ObjectType::PACKAGE, uu); });
        add_action_button("Merge 3D", bbox, br, [this](const UUID &uu) {
            auto pkg = pool.get_package(uu);
            for (const auto &it : pkg->models) {
                remote_box->merge_3d_model(it.second.filename);
            }
        });
    }

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

    auto canvas = Gtk::manage(new PreviewCanvas(pool, true));

    auto info_box = PackageInfoBox::create(pool);
    info_box->property_margin() = 10;
    info_box->signal_goto().connect(sigc::mem_fun(*this, &PoolNotebook::go_to));

    stack->add(*canvas, "preview");
    stack->add(*info_box, "info");
    info_box->unreference();

    paned->add2(*stack);
    paned->show_all();

    stack->set_visible_child(*canvas);

    br->signal_selected().connect([this, br, canvas, info_box] {
        auto sel = br->get_selected();
        if (!sel) {
            info_box->load(nullptr);
            canvas->clear();
            return;
        }
        info_box->load(pool.get_package(sel));
        canvas->load(ObjectType::PACKAGE, sel);
    });
    append_page(*paned, "Packages");
    install_search_once(paned, br);
}

} // namespace horizon
