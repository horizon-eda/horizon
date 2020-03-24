#include "export_file_chooser.hpp"
#include "util/util.hpp"

namespace horizon {
ExportFileChooser::ExportFileChooser()
{
}
ExportFileChooser::~ExportFileChooser()
{
}

void ExportFileChooser::attach(Gtk::Entry *en, Gtk::Button *bu, Gtk::Window *w)
{
    if (entry)
        return;
    entry = en;
    button = bu;
    window = w;

    button->signal_clicked().connect([this] {
        const char *title = "Select output file name";
        if (action == GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER) {
            title = "Select output folder";
        }
        GtkFileChooserNative *native =
                gtk_file_chooser_native_new(title, GTK_WINDOW(window->gobj()), action, "Select", "_Cancel");
        auto chooser = Glib::wrap(GTK_FILE_CHOOSER(native));
        prepare_chooser(chooser);
        chooser->add_shortcut_folder(project_dir);
        if (entry->get_text().size()) {
            if (Glib::path_is_absolute(entry->get_text()))
                chooser->set_filename(entry->get_text());
            else
                chooser->set_filename(Glib::build_filename(project_dir, entry->get_text()));
        }
        else {
            chooser->set_current_folder(project_dir);
        }
        if (gtk_native_dialog_run(GTK_NATIVE_DIALOG(native)) == GTK_RESPONSE_ACCEPT) {
            std::string filename = chooser->get_filename();
            prepare_filename(filename);
            auto fi = Gio::File::create_for_path(filename);
            auto pf = Gio::File::create_for_path(project_dir);
            if (fi->has_prefix(pf)) {
                std::string p = pf->get_relative_path(fi);
                replace_backslash(p);
                entry->set_text(p);
            }
            else {
                entry->set_text(filename);
            }

            s_signal_changed.emit();
        }
    });

    entry->signal_changed().connect([this] {
        if (filename_bound) {
            *filename_bound = entry->get_text();
            s_signal_changed.emit();
        }
    });
}

void ExportFileChooser::prepare_chooser(Glib::RefPtr<Gtk::FileChooser> chooser)
{
}

void ExportFileChooser::prepare_filename(std::string &filename)
{
}

void ExportFileChooser::set_project_dir(const std::string &d)
{
    project_dir = d;
}

const std::string &ExportFileChooser::get_project_dir() const
{
    return project_dir;
}

std::string ExportFileChooser::get_filename_abs(const std::string &filename)
{
    if (Glib::path_is_absolute(filename)) {
        return filename;
    }
    else {
        return Glib::build_filename(project_dir, filename);
    }
}

std::string ExportFileChooser::get_filename_abs()
{
    return get_filename_abs(get_filename());
}

std::string ExportFileChooser::get_filename()
{
    return entry->get_text();
}

void ExportFileChooser::bind_filename(std::string &filename)
{
    filename_bound = nullptr;
    entry->set_text(filename);
    filename_bound = &filename;
}

void ExportFileChooser::set_action(GtkFileChooserAction a)
{
    action = a;
}

} // namespace horizon
