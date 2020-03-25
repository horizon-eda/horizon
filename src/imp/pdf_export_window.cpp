#include "pdf_export_window.hpp"
#include "util/util.hpp"
#include "document/idocument_board.hpp"
#include "document/idocument_schematic.hpp"
#include "export_pdf/export_pdf.hpp"
#include "export_pdf/export_pdf_board.hpp"
#include "util/gtk_util.hpp"
#include "util/util.hpp"
#include "widgets/spin_button_dim.hpp"
#include "common/layer_provider.hpp"
#include <podofo/podofo.h>
#ifdef G_OS_WIN32
#undef DELETE
#undef DUPLICATE
#undef ERROR
#endif
#include <thread>

namespace horizon {

class PDFLayerEditor : public Gtk::Box, public Changeable {
public:
    PDFLayerEditor(BaseObjectType *cobject, const Glib::RefPtr<Gtk::Builder> &x, PDFExportWindow *pa,
                   PDFExportSettings::Layer *la);
    static PDFLayerEditor *create(PDFExportWindow *pa, PDFExportSettings::Layer *la);


private:
    PDFExportWindow *parent;
    Gtk::CheckButton *layer_checkbutton = nullptr;
    Gtk::RadioButton *layer_fill = nullptr;
    Gtk::RadioButton *layer_outline = nullptr;
    Gtk::ColorButton *layer_color = nullptr;

    PDFExportSettings::Layer *layer = nullptr;
};

PDFLayerEditor::PDFLayerEditor(BaseObjectType *cobject, const Glib::RefPtr<Gtk::Builder> &x, PDFExportWindow *pa,
                               PDFExportSettings::Layer *la)
    : Gtk::Box(cobject), parent(pa), layer(la)
{
    x->get_widget("layer_checkbutton", layer_checkbutton);
    x->get_widget("layer_fill", layer_fill);
    x->get_widget("layer_outline", layer_outline);
    x->get_widget("layer_color", layer_color);
    parent->sg_layer_name->add_widget(*layer_checkbutton);

    layer_checkbutton->set_label(parent->core->get_layer_provider()->get_layers().at(layer->layer).name);
    bind_widget(layer_checkbutton, layer->enabled);
    layer_checkbutton->signal_toggled().connect([this] { s_signal_changed.emit(); });
    {
        std::map<PDFExportSettings::Layer::Mode, Gtk::RadioButton *> widgets = {
                {PDFExportSettings::Layer::Mode::FILL, layer_fill},
                {PDFExportSettings::Layer::Mode::OUTLINE, layer_outline}};
        bind_widget<PDFExportSettings::Layer::Mode>(widgets, layer->mode,
                                                    [this](auto mode) { s_signal_changed.emit(); });
    }
    bind_widget(layer_color, layer->color, [this](auto color) { s_signal_changed.emit(); });
}

PDFLayerEditor *PDFLayerEditor::create(PDFExportWindow *pa, PDFExportSettings::Layer *la)
{
    PDFLayerEditor *w;
    Glib::RefPtr<Gtk::Builder> x = Gtk::Builder::create();
    x->add_from_resource("/org/horizon-eda/horizon/imp/pdf_export.ui", "layer_editor");
    x->get_widget_derived("layer_editor", w, pa, la);
    w->reference();
    return w;
}

void PDFExportWindow::MyExportFileChooser::prepare_chooser(Glib::RefPtr<Gtk::FileChooser> chooser)
{
    auto filter = Gtk::FileFilter::create();
    filter->set_name("PDF documents");
    filter->add_pattern("*.pdf");
    chooser->add_filter(filter);
}

void PDFExportWindow::MyExportFileChooser::prepare_filename(std::string &filename)
{
    if (!endswith(filename, ".pdf")) {
        filename += ".pdf";
    }
}

PDFExportWindow *PDFExportWindow::create(Gtk::Window *p, IDocument *c, PDFExportSettings &s, const std::string &pd)
{
    PDFExportWindow *w;
    Glib::RefPtr<Gtk::Builder> x = Gtk::Builder::create();
    x->add_from_resource("/org/horizon-eda/horizon/imp/pdf_export.ui", "window");
    x->get_widget_derived("window", w, c, s, pd);
    w->set_transient_for(*p);
    return w;
}

PDFExportWindow::PDFExportWindow(BaseObjectType *cobject, const Glib::RefPtr<Gtk::Builder> &x, IDocument *c,
                                 PDFExportSettings &s, const std::string &pd)
    : Gtk::Window(cobject), core(c), settings(s)
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
    x->get_widget("layers_box", layers_box);
    label_set_tnum(progress_label);

    sg_layer_name = Gtk::SizeGroup::create(Gtk::SIZE_GROUP_HORIZONTAL);


    int top = 1;
    {
        auto sp = Gtk::manage(new SpinButtonDim);
        sp->set_range(0, .5_mm);
        bind_widget(sp, settings.min_line_width);
        sp->signal_value_changed().connect([this] { s_signal_changed.emit(); });
        grid_attach_label_and_widget(grid, "Min. line width", sp, top);
    }

    if (dynamic_cast<IDocumentSchematic *>(core)) {
        Gtk::ScrolledWindow *sc;
        x->get_widget("layers_sc", sc);
        sc->set_visible(false);
    }
    else {
        {
            auto box = make_boolean_ganged_switch(settings.reverse_layers, "Normal", "Reversed",
                                                  [this](auto v) { s_signal_changed.emit(); });

            grid_attach_label_and_widget(grid, "Layer order", box, top);
        }
        {
            auto box = make_boolean_ganged_switch(settings.mirror, "Normal", "Mirrored",
                                                  [this](auto v) { s_signal_changed.emit(); });

            grid_attach_label_and_widget(grid, "Orientation", box, top);
        }
    }

    export_button->signal_clicked().connect(sigc::mem_fun(*this, &PDFExportWindow::generate));

    export_filechooser.attach(filename_entry, filename_button, this);
    export_filechooser.set_project_dir(pd);
    export_filechooser.bind_filename(settings.output_filename);
    export_filechooser.signal_changed().connect([this] {
        s_signal_changed.emit();
        update_export_button();
    });

    status_dispatcher.attach(progress_label);
    status_dispatcher.attach(progress_bar);
    status_dispatcher.attach(spinner);
    status_dispatcher.attach(progress_revealer);
    status_dispatcher.signal_notified().connect([this](const StatusDispatcher::Notification &n) {
        is_busy = n.status == StatusDispatcher::Status::BUSY;
        update_export_button();
        header->set_show_close_button(!is_busy);
    });

    signal_delete_event().connect([this](GdkEventAny *ev) { return is_busy; });
    reload_layers();
    update_export_button();
}

void PDFExportWindow::reload_layers()
{
    if (n_layers == settings.layers.size())
        return;
    else
        n_layers = settings.layers.size();

    {
        auto children = layers_box->get_children();
        for (auto ch : children)
            delete ch;
    }
    std::vector<PDFExportSettings::Layer *> layers_sorted;
    layers_sorted.reserve(settings.layers.size());
    for (auto &la : settings.layers) {
        layers_sorted.push_back(&la.second);
    }
    std::sort(layers_sorted.begin(), layers_sorted.end(),
              [](const auto a, const auto b) { return b->layer < a->layer; });

    for (auto la : layers_sorted) {
        auto ed = PDFLayerEditor::create(this, la);
        ed->signal_changed().connect([this] { s_signal_changed.emit(); });
        layers_box->add(*ed);
        ed->show();
        ed->unreference();
    }
}

void PDFExportWindow::generate()
{
    if (settings.output_filename.size() == 0 || is_busy)
        return;
    status_dispatcher.reset("Starting...");
    PDFExportSettings my_settings = settings;
    my_settings.output_filename = export_filechooser.get_filename_abs();
    std::thread thr(&PDFExportWindow::export_thread, this, my_settings);
    thr.detach();
}

void PDFExportWindow::update_export_button()
{
    std::string txt;
    if (!is_busy) {
        if (settings.output_filename.size() == 0) {
            txt = "output filename not set";
        }
    }
    else {
        txt = "busy";
    }
    widget_set_insensitive_tooltip(*export_button, txt);
}

void PDFExportWindow::export_thread(PDFExportSettings s)
{
    try {
        if (auto core_sch = dynamic_cast<IDocumentSchematic *>(core)) {
            export_pdf(*core_sch->get_schematic(), s, [this](std::string st, double p) {
                status_dispatcher.set_status(StatusDispatcher::Status::BUSY, st, p);
            });
            status_dispatcher.set_status(StatusDispatcher::Status::DONE, "Done", 1);
        }
        else if (auto core_brd = dynamic_cast<IDocumentBoard *>(core)) {
            export_pdf(*core_brd->get_board(), s, [this](std::string st, double p) {
                status_dispatcher.set_status(StatusDispatcher::Status::BUSY, st, p);
            });
            status_dispatcher.set_status(StatusDispatcher::Status::DONE, "Done", 1);
        }
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
