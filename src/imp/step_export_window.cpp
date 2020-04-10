#include "step_export_window.hpp"
#include "export_step/export_step.hpp"
#include "board/step_export_settings.hpp"
#include "util/util.hpp"
#include "util/gtk_util.hpp"
#include "document/idocument_board.hpp"
#include <thread>

namespace horizon {

void StepExportWindow::MyExportFileChooser::prepare_chooser(Glib::RefPtr<Gtk::FileChooser> chooser)
{
    auto filter = Gtk::FileFilter::create();
    filter->set_name("STEP models");
    filter->add_pattern("*.step");
    filter->add_pattern("*.stp");
    chooser->add_filter(filter);
}

void StepExportWindow::MyExportFileChooser::prepare_filename(std::string &filename)
{
    if (!endswith(filename, ".step") && !endswith(filename, ".stp")) {
        filename += ".step";
    }
}

StepExportWindow *StepExportWindow::create(Gtk::Window *p, IDocumentBoard *c, const std::string &project_dir)
{
    StepExportWindow *w;
    Glib::RefPtr<Gtk::Builder> x = Gtk::Builder::create();
    x->add_from_resource("/org/horizon-eda/horizon/imp/step_export.ui");
    x->get_widget_derived("window", w, c, project_dir);
    w->set_transient_for(*p);
    return w;
}

StepExportWindow::StepExportWindow(BaseObjectType *cobject, const Glib::RefPtr<Gtk::Builder> &x, IDocumentBoard *c,
                                   const std::string &project_dir)
    : Gtk::Window(cobject), core(c), settings(*core->get_step_export_settings())
{
    set_modal(true);
    x->get_widget("header", header);
    x->get_widget("spinner", spinner);
    x->get_widget("export_button", export_button);
    x->get_widget("filename_button", filename_button);
    x->get_widget("filename_entry", filename_entry);
    x->get_widget("prefix_entry", prefix_entry);
    x->get_widget("log_textview", log_textview);
    x->get_widget("include_3d_models_switch", include_3d_models_switch);

    export_button->signal_clicked().connect(sigc::mem_fun(*this, &StepExportWindow::generate));

    export_filechooser.attach(filename_entry, filename_button, this);
    export_filechooser.set_project_dir(project_dir);
    export_filechooser.bind_filename(settings.filename);
    export_filechooser.signal_changed().connect([this] {
        s_signal_changed.emit();
        update_export_button();
    });

    bind_widget(include_3d_models_switch, settings.include_3d_models);
    bind_widget(prefix_entry, settings.prefix);

    include_3d_models_switch->property_active().signal_changed().connect([this] { s_signal_changed.emit(); });
    prefix_entry->signal_changed().connect([this] { s_signal_changed.emit(); });

    tag = log_textview->get_buffer()->create_tag();
    tag->property_font_features() = "tnum 1";
    tag->property_font_features_set() = true;


    export_dispatcher.connect([this] {
        std::lock_guard<std::mutex> guard(msg_queue_mutex);
        auto buffer = log_textview->get_buffer();
        while (msg_queue.size()) {
            const std::string &msg = msg_queue.front();
            buffer->insert(buffer->end(), msg + "\n");
            auto adj = log_textview->get_vadjustment();
            adj->set_value(adj->get_upper());
            msg_queue.pop_front();
        }
        {
            Gtk::TextIter ibegin, iend;
            buffer->get_bounds(ibegin, iend);
            buffer->remove_all_tags(ibegin, iend);
            buffer->apply_tag(tag, ibegin, iend);
        }
        if (export_running == false)
            set_is_busy(false);
    });
    update_export_button();
}

void StepExportWindow::generate()
{
    if (export_running)
        return;
    if (settings.filename.size() == 0)
        return;
    set_is_busy(true);
    log_textview->get_buffer()->set_text("");
    msg_queue.clear();
    export_running = true;
    STEPExportSettings my_settings = settings;
    my_settings.filename = export_filechooser.get_filename_abs();
    std::thread thr(&StepExportWindow::export_thread, this, my_settings);
    thr.detach();
}

void StepExportWindow::export_thread(STEPExportSettings my_settings)
{
    try {
        export_step(
                my_settings.filename, *core->get_board(), *core->get_pool(), my_settings.include_3d_models,
                [this](const std::string &msg) {
                    {
                        std::lock_guard<std::mutex> guard(msg_queue_mutex);
                        msg_queue.push_back(msg);
                    }
                    export_dispatcher.emit();
                },
                core->get_colors(), my_settings.prefix);
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

    export_running = false;
    export_dispatcher.emit();
}

void StepExportWindow::set_can_export(bool v)
{
    can_export = v;
    update_export_button();
}

void StepExportWindow::update_export_button()
{
    std::string txt;
    if (!export_running) {
        if (settings.filename.size() == 0) {
            txt = "output filename not set";
        }
    }
    else {
        txt = "busy";
    }
    widget_set_insensitive_tooltip(*export_button, txt);
}

void StepExportWindow::set_is_busy(bool v)
{
    update_export_button();
    spinner->property_active() = v;
    header->set_show_close_button(!v);
}

} // namespace horizon
