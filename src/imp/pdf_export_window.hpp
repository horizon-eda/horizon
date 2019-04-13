#pragma once
#include <gtkmm.h>
#include <array>
#include <set>
#include "util/status_dispatcher.hpp"
#include "util/changeable.hpp"
#include "schematic/pdf_export_settings.hpp"

namespace horizon {

class PDFExportWindow : public Gtk::Window, public Changeable {

public:
    PDFExportWindow(BaseObjectType *cobject, const Glib::RefPtr<Gtk::Builder> &x, const class Schematic &sch,
                    class PDFExportSettings &s);
    static PDFExportWindow *create(Gtk::Window *p, const class Schematic &sch, class PDFExportSettings &s);

    void set_can_export(bool v);
    void generate();

private:
    const class Schematic &sch;
    class PDFExportSettings &settings;

    Gtk::HeaderBar *header = nullptr;
    Gtk::Entry *filename_entry = nullptr;
    Gtk::Button *filename_button = nullptr;
    class SpinButtonDim *min_line_width_sp = nullptr;
    Gtk::Grid *grid = nullptr;

    Gtk::Button *export_button = nullptr;
    Gtk::Label *progress_label = nullptr;
    Gtk::ProgressBar *progress_bar = nullptr;
    Gtk::Revealer *progress_revealer = nullptr;

    Gtk::Spinner *spinner = nullptr;

    StatusDispatcher status_dispatcher;
    bool is_busy = false;


    void export_thread(PDFExportSettings settings);
};
} // namespace horizon
