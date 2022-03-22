#include "import_kicad_package_window.hpp"
#include "pool-prj-mgr/pool-prj-mgr-app_win.hpp"
#include "util/gtk_util.hpp"
#include "widgets/preview_canvas.hpp"
#include "sexpr/sexpr_parser.h"
#include "util/kicad_package_parser.hpp"
#include "util/util.hpp"
#include "pool/pool.hpp"
#include "util/str_util.hpp"
#include "widgets/log_view.hpp"
#include "nlohmann/json.hpp"

namespace horizon {
ImportKiCadPackageWindow *ImportKiCadPackageWindow::create(PoolProjectManagerAppWindow &aw)
{
    ImportKiCadPackageWindow *w;
    Glib::RefPtr<Gtk::Builder> x = Gtk::Builder::create();
    x->add_from_resource("/org/horizon-eda/horizon/pool-prj-mgr/pool-mgr/import_kicad_package_window.ui");
    x->get_widget_derived("window", w, aw);

    w->set_transient_for(aw);

    return w;
}

ImportKiCadPackageWindow::ImportKiCadPackageWindow(BaseObjectType *cobject, const Glib::RefPtr<Gtk::Builder> &x,
                                                   PoolProjectManagerAppWindow &aw)
    : Gtk::Window(cobject), appwin(aw), window_state_store(this, "import-kicad-package-window")
{
    GET_WIDGET(chooser_widget);
    GET_WIDGET(package_name_label);
    GET_WIDGET(import_error_box);
    GET_WIDGET(import_button);

    static const std::vector<std::string> default_paths = {"/usr/share/kicad/modules/", "/usr/share/kicad/footprints"};
    for (const auto &path : default_paths) {
        if (Glib::file_test(path, Glib::FILE_TEST_IS_DIR)) {
            chooser_widget->add_shortcut_folder(path);
            chooser_widget->set_current_folder(path);
        }
    }

    {
        Gtk::Box *canvas_box;
        GET_WIDGET(canvas_box);
        canvas = Gtk::manage(new PreviewCanvas(*aw.pool, true));
        canvas->show();
        canvas_box->pack_start(*canvas, true, true, 0);
    }


    log_view = Gtk::manage(new LogView);
    import_error_box->pack_start(*log_view, true, true, 0);
    log_view->show();

    chooser_widget->signal_selection_changed().connect(sigc::mem_fun(*this, &ImportKiCadPackageWindow::update));
    chooser_widget->signal_file_activated().connect(sigc::mem_fun(*this, &ImportKiCadPackageWindow::handle_import));
    import_button->signal_clicked().connect(sigc::mem_fun(*this, &ImportKiCadPackageWindow::handle_import));
    import_button->set_sensitive(false);

    {
        Gtk::Paned *paned1;
        GET_WIDGET(paned1);
        paned1_state_store.emplace(paned1, "import_kicad_package_window_h");
    }
    {
        Gtk::Paned *paned2;
        GET_WIDGET(paned2);
        paned2_state_store.emplace(paned2, "import_kicad_package_window_v");
    }
}

static std::string slurp_from_file(const std::string &filename)
{
    auto ifs = make_ifstream(filename);
    if (!ifs.is_open()) {
        throw std::runtime_error("not opened");
    }
    std::stringstream text;
    text << ifs.rdbuf();
    return text.str();
}


void ImportKiCadPackageWindow::update()
{
    const auto filename = chooser_widget->get_filename();
    if (filename.size() && Glib::file_test(filename, Glib::FILE_TEST_IS_REGULAR)) {
        std::string ex_str;
        log_view->clear();
        try {
            uint64_t log_seq = 0;
            SEXPR::PARSER parser;
            std::unique_ptr<SEXPR::SEXPR> sexpr_data(parser.Parse(slurp_from_file(filename)));
            package.emplace(UUID::random());
            KiCadPackageParser kp(*package, *appwin.pool);
            kp.set_log_cb([this, &log_seq](const auto &msg, const auto &detail) {
                Logger::Item it(log_seq++, Logger::Level::WARNING, msg, Logger::Domain::IMPORT, detail);
                log_view->push_log(it);
            });
            const auto meta = kp.parse(sexpr_data.get());
            package->name = meta.name;
            package->tags = meta.tags;
            package->expand();
            canvas->load(*package);
            package_name_label->set_text(package->name + "\n" + meta.descr);
            import_error_box->set_visible(log_seq);
            import_button->set_sensitive(true);
        }
        catch (const std::exception &e) {
            ex_str = e.what();
        }
        catch (...) {
            ex_str = "unknown exception";
        }
        if (ex_str.size()) {
            package_name_label->set_text("None");
            Logger::Item it(0, Logger::Level::CRITICAL, "exception: " + ex_str, Logger::Domain::IMPORT);
            log_view->push_log(it);
            import_error_box->set_visible(true);
            canvas->clear();
            import_button->set_sensitive(false);
        }
    }
    else {
        canvas->clear();
        package_name_label->set_text("None");
        import_error_box->set_visible(false);
        import_button->set_sensitive(false);
    }
}

void ImportKiCadPackageWindow::handle_import()
{
    auto top = dynamic_cast<Gtk::Window *>(get_ancestor(GTK_TYPE_WINDOW));

    GtkFileChooserNative *native = gtk_file_chooser_native_new(
            "Save Package", top->gobj(), GTK_FILE_CHOOSER_ACTION_CREATE_FOLDER, "_Save", "_Cancel");
    auto chooser = Glib::wrap(GTK_FILE_CHOOSER(native));
    chooser->set_do_overwrite_confirmation(true);
    chooser->set_current_name(package.value().name);
    const auto base_path = appwin.pool->get_base_path();
    while (true) {
        chooser->set_current_folder(Glib::build_filename(base_path, "packages"));
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
                auto fi = Gio::File::create_for_path(Glib::build_filename(fn, "padstacks"));
                fi->make_directory_with_parents();
                const auto pkg_filename = Glib::build_filename(fn, "package.json");
                save_json_to_file(pkg_filename, package.value().serialize());
                appwin.spawn(PoolProjectManagerProcess::Type::IMP_PACKAGE, {pkg_filename});
                files_saved.push_back(pkg_filename);
                Gtk::Window::close();
            }
        }
        break;
    }
}

} // namespace horizon
