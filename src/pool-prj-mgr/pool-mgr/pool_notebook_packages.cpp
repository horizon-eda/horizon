#include "pool_notebook.hpp"
#include "duplicate/duplicate_part.hpp"
#include "util/util.hpp"
#include <nlohmann/json.hpp>
#include "pool-prj-mgr/pool-prj-mgr-app_win.hpp"
#include "widgets/pool_browser_package.hpp"
#include "pool_remote_box.hpp"
#include "widgets/package_info_box.hpp"
#include "util/win32_undef.hpp"
#include "widgets/preview_canvas.hpp"
#include "import_kicad_package_window.hpp"
#include "util/gtk_util.hpp"

namespace horizon {
void PoolNotebook::handle_edit_package(const UUID &uu)
{
    edit_item(ObjectType::PACKAGE, uu);
}

void PoolNotebook::handle_create_package()
{
    Package pkg(horizon::UUID::random());
    std::string fn = pool.get_tmp_filename(ObjectType::PACKAGE, pkg.uuid);
    save_json_to_file(fn, pkg.serialize());

    appwin.spawn(PoolProjectManagerProcess::Type::IMP_PACKAGE, {fn}, PoolProjectManagerAppWindow::SpawnFlags::TEMP);
}

void PoolNotebook::handle_create_padstack_for_package(const UUID &uu)
{
    if (!uu)
        return;

    auto pkg_filename = pool.get_filename(ObjectType::PACKAGE, uu);
    auto pkg_dir = Glib::path_get_dirname(pkg_filename);
    const auto padstack_dir = Glib::build_filename(pkg_dir, "padstacks");

    Padstack padstack(horizon::UUID::random());
    std::string fn = pool.get_tmp_filename(ObjectType::PADSTACK, padstack.uuid);
    save_json_to_file(fn, padstack.serialize());

    appwin.spawn(PoolProjectManagerProcess::Type::IMP_PADSTACK, {fn, padstack_dir},
                 PoolProjectManagerAppWindow::SpawnFlags::TEMP);
}

void PoolNotebook::handle_duplicate_package(const UUID &uu)
{
    if (!uu)
        return;
    auto top = dynamic_cast<Gtk::Window *>(get_ancestor(GTK_TYPE_WINDOW));

    GtkFileChooserNative *native =
            gtk_file_chooser_native_new("Save Package", top->gobj(), GTK_FILE_CHOOSER_ACTION_SAVE, "_Save", "_Cancel");
    auto chooser = Glib::wrap(GTK_FILE_CHOOSER(native));
    auto pkg_filename = Glib::build_filename(pool.get_base_path(),
                                             Glib::path_get_dirname(pool.get_rel_filename(ObjectType::PACKAGE, uu)));
    auto pkg_basename = Glib::path_get_basename(pkg_filename);
    auto pkg_dirname = Glib::path_get_dirname(pkg_filename);
    chooser->set_current_folder(pkg_dirname);
    chooser->set_current_name(pkg_basename + "-copy");


    std::vector<std::string> filenames;
    std::string new_pkg_filename;
    auto success = run_native_filechooser_with_retry(
            chooser, "Error creating package", [this, chooser, &filenames, &uu, &new_pkg_filename] {
                auto filename = chooser->get_filename();
                pool.check_filename_throw(ObjectType::PACKAGE, filename);
                DuplicatePartWidget::duplicate_package(pool, uu, filename, Glib::path_get_basename(filename),
                                                       &filenames);
                new_pkg_filename = Glib::build_filename(filename, "package.json");
            });

    if (success) {
        pool_update_done_cb = [this, new_pkg_filename] {
            appwin.spawn(PoolProjectManagerProcess::Type::IMP_PACKAGE, {new_pkg_filename});
        };
        pool_update(filenames);
    }
}

void PoolNotebook::handle_import_kicad_package()
{
    if (!import_kicad_package_window) {

        import_kicad_package_window = ImportKiCadPackageWindow::create(appwin);
        import_kicad_package_window->present();
        import_kicad_package_window->signal_hide().connect([this] {
            delete import_kicad_package_window;
            import_kicad_package_window = nullptr;
        });
    }

    else {
        import_kicad_package_window->present();
    }
}

void PoolNotebook::construct_packages()
{
    auto br = Gtk::manage(new PoolBrowserPackage(pool, false, "pool_notebook"));
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
    add_action_button("Import KiCad package", bbox, sigc::mem_fun(*this, &PoolNotebook::handle_import_kicad_package))
            ->get_style_context()
            ->add_class("suggested-action");
    add_merge_button(bbox, br);
    if (auto bu = add_merge_button(bbox, br, [this](const UUID &uu) {
            auto pkg = pool.get_package(uu);
            for (const auto &it : pkg->models) {
                remote_box->merge_3d_model(it.second.filename);
            }
        })) {
        bu->set_label("Merge 3D");
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
        info_box->load(pool.get_package(sel).get());
        canvas->load(ObjectType::PACKAGE, sel);
    });
    append_page(*paned, "Packages");
    install_search_once(paned, br);
    create_paned_state_store(paned, "packages");
}

} // namespace horizon
