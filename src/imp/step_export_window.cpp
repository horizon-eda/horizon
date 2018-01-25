#include "step_export_window.hpp"
#include "export_step/export_step.hpp"
#include <thread>

namespace horizon {

StepExportWindow *StepExportWindow::create(Gtk::Window *p, const Board *b, class Pool *po)
{
    StepExportWindow *w;
    Glib::RefPtr<Gtk::Builder> x = Gtk::Builder::create();
    x->add_from_resource("/net/carrotIndustries/horizon/imp/step_export.ui");
    x->get_widget_derived("window", w, b, po);
    w->set_transient_for(*p);
    return w;
}

StepExportWindow::StepExportWindow(BaseObjectType *cobject, const Glib::RefPtr<Gtk::Builder> &x, const Board *bo,
                                   Pool *p)
    : Gtk::Window(cobject), brd(bo), pool(p)
{
    set_modal(true);
    x->get_widget("header", header);
    x->get_widget("spinner", spinner);
    x->get_widget("export_button", export_button);
    x->get_widget("filename_button", filename_button);
    x->get_widget("filename_entry", filename_entry);
    x->get_widget("log_textview", log_textview);
    x->get_widget("include_3d_models_switch", include_3d_models_switch);

    export_button->signal_clicked().connect(sigc::mem_fun(this, &StepExportWindow::handle_export));

    filename_button->signal_clicked().connect([this] {
        GtkFileChooserNative *native = gtk_file_chooser_native_new("Select output directory", GTK_WINDOW(gobj()),
                                                                   GTK_FILE_CHOOSER_ACTION_SAVE, "Select", "_Cancel");
        auto chooser = Glib::wrap(GTK_FILE_CHOOSER(native));
        auto filter = Gtk::FileFilter::create();
        filter->set_name("STEP models");
        filter->add_pattern("*.step");
        filter->add_pattern("*.stp");
        chooser->add_filter(filter);
        chooser->set_filename(filename_entry->get_text());

        if (gtk_native_dialog_run(GTK_NATIVE_DIALOG(native)) == GTK_RESPONSE_ACCEPT) {
            filename_entry->set_text(chooser->get_filename());
        }
    });

    export_dispatcher.connect([this] {
        std::lock_guard<std::mutex> guard(msg_queue_mutex);
        while (msg_queue.size()) {
            const std::string &msg = msg_queue.front();
            auto buffer = log_textview->get_buffer();
            auto end_iter = buffer->get_iter_at_offset(-1);
            buffer->insert(end_iter, msg + "\n");
            auto adj = log_textview->get_vadjustment();
            adj->set_value(adj->get_upper());

            msg_queue.pop_front();
        }
        if (export_running == false)
            set_is_busy(false);
    });
}

void StepExportWindow::handle_export()
{
    std::string filename = filename_entry->get_text();
    if (filename.size() == 0)
        return;
    set_is_busy(true);
    log_textview->get_buffer()->set_text("");
    msg_queue.clear();
    export_running = true;
    auto include_models = include_3d_models_switch->get_active();
    std::thread thr(&StepExportWindow::export_thread, this, filename, include_models);
    thr.detach();
}

void StepExportWindow::export_thread(std::string filename, bool include_models)
{
    try {
        export_step(filename, *brd, *pool, include_models, [this](std::string msg) {
            {
                std::lock_guard<std::mutex> guard(msg_queue_mutex);
                msg_queue.push_back(msg);
            }
            export_dispatcher.emit();
        });
    }
    catch (const std::exception &e) {
        {
            std::lock_guard<std::mutex> guard(msg_queue_mutex);
            msg_queue.push_back(std::string("Error: ") + e.what());
        }
        export_dispatcher.emit();
    }
    catch (...) {
        {
            std::lock_guard<std::mutex> guard(msg_queue_mutex);
            msg_queue.push_back("Error: unknown error");
        }
        export_dispatcher.emit();
    }
    {
        std::lock_guard<std::mutex> guard(msg_queue_mutex);
        msg_queue.push_back("Done");
    }

    export_running = false;
    export_dispatcher.emit();
}

void StepExportWindow::set_can_export(bool v)
{
    export_button->set_sensitive(v);
}

void StepExportWindow::set_is_busy(bool v)
{
    export_button->set_sensitive(!v);
    spinner->property_active() = v;
    header->set_show_close_button(!v);
}

} // namespace horizon
