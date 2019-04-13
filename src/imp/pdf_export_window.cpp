#include "pdf_export_window.hpp"
#include "util/util.hpp"
#include "core/core_schematic.hpp"
#include "export_pdf/export_pdf.hpp"
#include "util/gtk_util.hpp"
#include "widgets/spin_button_dim.hpp"
#include <podofo/podofo.h>
#include <thread>

namespace horizon {

PDFExportWindow *PDFExportWindow::create(Gtk::Window *p, const Schematic &c, PDFExportSettings &s)
{
    PDFExportWindow *w;
    Glib::RefPtr<Gtk::Builder> x = Gtk::Builder::create();
    x->add_from_resource("/net/carrotIndustries/horizon/imp/pdf_export.ui");
    x->get_widget_derived("window", w, c, s);
    w->set_transient_for(*p);
    return w;
}

PDFExportWindow::PDFExportWindow(BaseObjectType *cobject, const Glib::RefPtr<Gtk::Builder> &x, const Schematic &c,
                                 PDFExportSettings &s)
    : Gtk::Window(cobject), sch(c), settings(s)
{
    set_modal(true);
    x->get_widget("header", header);
    x->get_widget("spinner", spinner);
    x->get_widget("export_button", export_button);
    x->get_widget("filename_button", filename_button);
    x->get_widget("filename_entry", filename_entry);
    x->get_widget("progress_label", progress_label);
    x->get_widget("progress_bar", progress_bar);
    x->get_widget("progress_revealer", progress_revealer);
    x->get_widget("grid", grid);
    label_set_tnum(progress_label);

    {
        int top = 1;
        auto sp = Gtk::manage(new SpinButtonDim);
        sp->set_range(0, .5_mm);
        bind_widget(sp, settings.min_line_width);
        grid_attach_label_and_widget(grid, "Min. line width", sp, top);
    }


    export_button->signal_clicked().connect(sigc::mem_fun(*this, &PDFExportWindow::generate));

    filename_button->signal_clicked().connect([this] {
        GtkFileChooserNative *native = gtk_file_chooser_native_new("Select output file name", GTK_WINDOW(gobj()),
                                                                   GTK_FILE_CHOOSER_ACTION_SAVE, "Select", "_Cancel");
        auto chooser = Glib::wrap(GTK_FILE_CHOOSER(native));
        auto filter = Gtk::FileFilter::create();
        filter->set_name("PDF documents");
        filter->add_pattern("*.pdf");
        chooser->add_filter(filter);
        chooser->set_filename(filename_entry->get_text());

        if (gtk_native_dialog_run(GTK_NATIVE_DIALOG(native)) == GTK_RESPONSE_ACCEPT) {
            std::string filename = chooser->get_filename();
            if (!endswith(filename, ".pdf")) {
                filename += ".step";
            }
            filename_entry->set_text(filename);
            s_signal_changed.emit();
        }
    });
    bind_widget(filename_entry, settings.output_filename, [this](std::string &) { s_signal_changed.emit(); });

    status_dispatcher.attach(progress_label);
    status_dispatcher.attach(progress_bar);
    status_dispatcher.attach(spinner);
    status_dispatcher.attach(progress_revealer);
    status_dispatcher.signal_notified().connect([this](const StatusDispatcher::Notification &n) {
        is_busy = n.status == StatusDispatcher::Status::BUSY;
        export_button->set_sensitive(!is_busy);
        header->set_show_close_button(!is_busy);
    });

    signal_delete_event().connect([this](GdkEventAny *ev) { return is_busy; });
}

void PDFExportWindow::generate()
{
    std::string filename = filename_entry->get_text();
    if (filename.size() == 0 || is_busy)
        return;
    status_dispatcher.reset("Starting...");

    std::thread thr(&PDFExportWindow::export_thread, this, settings);
    thr.detach();
}

void PDFExportWindow::export_thread(PDFExportSettings s)
{
    try {
        export_pdf(sch, s, [this](std::string st, double p) {
            status_dispatcher.set_status(StatusDispatcher::Status::BUSY, st, p);
        });
        status_dispatcher.set_status(StatusDispatcher::Status::DONE, "Done", 1);
    }
    catch (const std::exception &e) {
        status_dispatcher.set_status(StatusDispatcher::Status::ERROR, e.what(), 0);
    }
    catch (const PoDoFo::PdfError &e) {
        status_dispatcher.set_status(StatusDispatcher::Status::ERROR, e.what(), 0);
    }
    catch (...) {
        status_dispatcher.set_status(StatusDispatcher::Status::ERROR, "Unknown error", 0);
    }
}


} // namespace horizon
