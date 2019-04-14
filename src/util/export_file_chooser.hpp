#pragma once
#include <gtkmm.h>
#include <string>
#include "changeable.hpp"

namespace horizon {
class ExportFileChooser : public Changeable {
public:
    ExportFileChooser();
    virtual ~ExportFileChooser();
    void attach(Gtk::Entry *en, Gtk::Button *bu, Gtk::Window *w);
    void set_project_dir(const std::string &d);
    const std::string &get_project_dir() const;
    std::string get_filename_abs(const std::string &filename);
    std::string get_filename_abs();
    std::string get_filename();
    void bind_filename(std::string &filename);
    void set_action(GtkFileChooserAction action);

protected:
    virtual void prepare_chooser(Glib::RefPtr<Gtk::FileChooser> chooser);
    virtual void prepare_filename(std::string &filename);

private:
    Gtk::Button *button = nullptr;
    Gtk::Entry *entry = nullptr;
    Gtk::Window *window = nullptr;
    std::string project_dir;
    std::string *filename_bound = nullptr;
    GtkFileChooserAction action = GTK_FILE_CHOOSER_ACTION_SAVE;
};

} // namespace horizon
