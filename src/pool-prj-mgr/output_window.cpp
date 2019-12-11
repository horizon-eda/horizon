#include "output_window.hpp"
#include "util/util.hpp"

namespace horizon {
OutputWindow *OutputWindow::create()
{
    OutputWindow *w;
    Glib::RefPtr<Gtk::Builder> x = Gtk::Builder::create();
    x->add_from_resource("/org/horizon-eda/horizon/pool-prj-mgr/output_window.ui");
    x->get_widget_derived("window", w);

    return w;
}

OutputWindow::OutputWindow(BaseObjectType *cobject, const Glib::RefPtr<Gtk::Builder> &x) : Gtk::Window(cobject)
{
    x->get_widget("view_stderr", view_stderr);
    x->get_widget("view_stdout", view_stdout);
    x->get_widget("stack", stack);

    Gtk::Button *button_clear;
    x->get_widget("button_clear", button_clear);
    button_clear->signal_clicked().connect([this] { get_view()->get_buffer()->set_text(""); });

    Gtk::Button *button_save;
    x->get_widget("button_save", button_save);
    button_save->signal_clicked().connect([this] {
        auto txt = get_view()->get_buffer()->get_text();
        std::string filename;
        {
            GtkFileChooserNative *native = gtk_file_chooser_native_new(
                    "Save output", GTK_WINDOW(this->gobj()), GTK_FILE_CHOOSER_ACTION_SAVE, "_Save", "_Cancel");
            auto chooser = Glib::wrap(GTK_FILE_CHOOSER(native));
            chooser->set_do_overwrite_confirmation(true);
            chooser->set_current_name(stack->get_visible_child_name() + ".txt");

            if (gtk_native_dialog_run(GTK_NATIVE_DIALOG(native)) == GTK_RESPONSE_ACCEPT) {
                filename = chooser->get_filename();
            }
        }
        if (filename.size()) {
            while (1) {
                std::string error_str;
                try {
                    auto ofs = make_ofstream(filename);
                    if (!ofs.is_open())
                        throw std::runtime_error("not opened");
                    ofs << txt;
                    break;
                }
                catch (const std::exception &e) {
                    error_str = e.what();
                }
                catch (...) {
                    error_str = "unknown error";
                }
                if (error_str.size()) {
                    Gtk::MessageDialog dia(*this, "Save error", false, Gtk::MESSAGE_ERROR, Gtk::BUTTONS_NONE);
                    dia.set_secondary_text(error_str);
                    dia.add_button("Cancel", Gtk::RESPONSE_CANCEL);
                    dia.add_button("Retry", 1);
                    if (dia.run() != 1)
                        break;
                }
            }
        }
    });
}

Gtk::TextView *OutputWindow::get_view()
{
    if (stack->get_visible_child_name() == "stdout") {
        return view_stdout;
    }
    else {
        return view_stderr;
    }
}

void OutputWindow::handle_output(const std::string &line, bool err)
{
    Gtk::TextView *view = err ? view_stderr : view_stdout;
    auto buffer = view->get_buffer();
    auto end_iter = buffer->get_iter_at_offset(-1);
    buffer->insert(end_iter, line + "\n");

    auto adj = view->get_vadjustment();
    adj->set_value(adj->get_upper());
}

void OutputWindow::clear_all()
{
    view_stderr->get_buffer()->set_text("");
    view_stdout->get_buffer()->set_text("");
}

} // namespace horizon
