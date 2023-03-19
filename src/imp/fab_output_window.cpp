#include "fab_output_window.hpp"
#include "board/board.hpp"
#include "export_gerber/gerber_export.hpp"
#include "export_odb/odb_export.hpp"
#include "util/gtk_util.hpp"
#include "widgets/spin_button_dim.hpp"
#include "document/idocument_board.hpp"
#include "rules/rules_with_core.hpp"
#include "rules/cache.hpp"
#include "util/util.hpp"
#include <range/v3/view.hpp>

namespace horizon {

class GerberLayerEditor : public Gtk::Box, public Changeable {
public:
    GerberLayerEditor(BaseObjectType *cobject, const Glib::RefPtr<Gtk::Builder> &x, FabOutputWindow &pa,
                      GerberOutputSettings::GerberLayer &la);
    static GerberLayerEditor *create(FabOutputWindow &pa, GerberOutputSettings::GerberLayer &la);
    FabOutputWindow &parent;

private:
    Gtk::CheckButton *gerber_layer_checkbutton = nullptr;
    Gtk::Entry *gerber_layer_filename_entry = nullptr;

    GerberOutputSettings::GerberLayer &layer;
};

GerberLayerEditor::GerberLayerEditor(BaseObjectType *cobject, const Glib::RefPtr<Gtk::Builder> &x, FabOutputWindow &pa,
                                     GerberOutputSettings::GerberLayer &la)
    : Gtk::Box(cobject), parent(pa), layer(la)
{
    GET_WIDGET(gerber_layer_checkbutton);
    GET_WIDGET(gerber_layer_filename_entry);
    parent.sg_layer_name->add_widget(*gerber_layer_checkbutton);

    gerber_layer_checkbutton->set_label(parent.brd.get_layers().at(layer.layer).name);
    bind_widget(gerber_layer_checkbutton, layer.enabled);
    gerber_layer_checkbutton->signal_toggled().connect([this] { s_signal_changed.emit(); });
    bind_widget(gerber_layer_filename_entry, layer.filename);
    gerber_layer_filename_entry->signal_changed().connect([this] { s_signal_changed.emit(); });
}

GerberLayerEditor *GerberLayerEditor::create(FabOutputWindow &pa, GerberOutputSettings::GerberLayer &la)
{
    GerberLayerEditor *w;
    Glib::RefPtr<Gtk::Builder> x = Gtk::Builder::create();
    x->add_from_resource("/org/horizon-eda/horizon/imp/fab_output.ui", "gerber_layer_editor");
    x->get_widget_derived("gerber_layer_editor", w, pa, la);
    w->reference();
    return w;
}

class DrillEditor : public Gtk::Box, public Changeable {
public:
    DrillEditor(BaseObjectType *cobject, const Glib::RefPtr<Gtk::Builder> &x, FabOutputWindow &pa,
                const std::string &label, std::string &filename);
    static DrillEditor *create(FabOutputWindow &pa, const std::string &label, std::string &filename);
    FabOutputWindow &parent;

private:
    Gtk::Label *drill_span_label = nullptr;
    Gtk::Entry *drill_filename_entry = nullptr;
};

DrillEditor::DrillEditor(BaseObjectType *cobject, const Glib::RefPtr<Gtk::Builder> &x, FabOutputWindow &pa,
                         const std::string &label, std::string &filename)
    : Gtk::Box(cobject), parent(pa)
{
    GET_WIDGET(drill_span_label);
    GET_WIDGET(drill_filename_entry);
    parent.sg_drill_span_name->add_widget(*drill_span_label);
    drill_span_label->set_text(label);

    bind_widget(drill_filename_entry, filename);
    drill_filename_entry->signal_changed().connect([this] { s_signal_changed.emit(); });
}

DrillEditor *DrillEditor::create(FabOutputWindow &pa, const std::string &label, std::string &fn)
{
    DrillEditor *w;
    Glib::RefPtr<Gtk::Builder> x = Gtk::Builder::create();
    x->add_from_resource("/org/horizon-eda/horizon/imp/fab_output.ui", "drill_editor");
    x->get_widget_derived("drill_editor", w, pa, label, fn);
    w->reference();
    return w;
}

FabOutputWindow::ODBExportFileChooserFilename::ODBExportFileChooserFilename(const ODBOutputSettings &set)
    : settings(set)
{
}


void FabOutputWindow::ODBExportFileChooserFilename::prepare_chooser(Glib::RefPtr<Gtk::FileChooser> chooser)
{
    auto filter = Gtk::FileFilter::create();
    if (settings.format == ODBOutputSettings::Format::TGZ) {
        filter->set_name("Tarballs");
        filter->add_pattern("*.tgz");
    }
    else if (settings.format == ODBOutputSettings::Format::ZIP) {
        filter->set_name("ZIP archives");
        filter->add_pattern("*.zip");
    }
    chooser->add_filter(filter);
}

void FabOutputWindow::ODBExportFileChooserFilename::prepare_filename(std::string &filename)
{
    if (settings.format == ODBOutputSettings::Format::TGZ) {
        if (!endswith(filename, ".tgz")) {
            filename += ".tgz";
        }
    }
    else if (settings.format == ODBOutputSettings::Format::ZIP) {
        if (!endswith(filename, ".zip")) {
            filename += ".zip";
        }
    }
}

FabOutputWindow::FabOutputWindow(BaseObjectType *cobject, const Glib::RefPtr<Gtk::Builder> &x, IDocumentBoard &c,
                                 const std::string &project_dir)
    : Gtk::Window(cobject), core(c), brd(*core.get_board()), settings(core.get_gerber_output_settings()),
      odb_settings(core.get_odb_output_settings()), odb_export_filechooser_filename(odb_settings),
      state_store(this, "imp-fab-output")
{
    GET_WIDGET(gerber_layers_box);
    GET_WIDGET(prefix_entry);
    GET_WIDGET(directory_entry);
    GET_WIDGET(npth_filename_entry);
    GET_WIDGET(pth_filename_entry);
    GET_WIDGET(npth_filename_label);
    GET_WIDGET(pth_filename_label);
    GET_WIDGET(generate_button);
    GET_WIDGET(directory_button);
    GET_WIDGET(drill_mode_combo);
    GET_WIDGET(blind_buried_box);
    GET_WIDGET(blind_buried_drills_box);
    GET_WIDGET(log_textview);
    GET_WIDGET(zip_output_switch);
    GET_WIDGET(done_label);
    GET_WIDGET(done_revealer);
    GET_WIDGET(done_close_button);
    GET_WIDGET(stack);
    GET_WIDGET(odb_filename_entry);
    GET_WIDGET(odb_filename_button);
    GET_WIDGET(odb_filename_box);
    GET_WIDGET(odb_filename_label);
    GET_WIDGET(odb_directory_entry);
    GET_WIDGET(odb_directory_button);
    GET_WIDGET(odb_directory_box);
    GET_WIDGET(odb_directory_label);
    GET_WIDGET(odb_format_tgz_rb);
    GET_WIDGET(odb_format_directory_rb);
    GET_WIDGET(odb_format_zip_rb);
    GET_WIDGET(odb_job_name_entry);
    GET_WIDGET(odb_done_label);
    GET_WIDGET(odb_done_revealer);
    GET_WIDGET(odb_done_close_button);

    done_revealer_controller.attach(done_revealer, done_label, done_close_button);
    odb_done_revealer_controller.attach(odb_done_revealer, odb_done_label, odb_done_close_button);

    export_filechooser.attach(directory_entry, directory_button, this);
    export_filechooser.set_project_dir(project_dir);
    export_filechooser.bind_filename(settings.output_directory);
    export_filechooser.signal_changed().connect([this] {
        s_signal_changed.emit();
        update_export_button();
    });
    export_filechooser.set_action(GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER);

    bind_widget(prefix_entry, settings.prefix, [this](std::string &) {
        s_signal_changed.emit();
        update_export_button();
    });
    prefix_entry->signal_changed().connect([this] { s_signal_changed.emit(); });
    bind_widget(npth_filename_entry, settings.drill_npth_filename, [this](std::string &) {
        s_signal_changed.emit();
        update_export_button();
    });
    bind_widget(pth_filename_entry, settings.drill_pth_filename, [this](std::string &) {
        s_signal_changed.emit();
        update_export_button();
    });
    bind_widget(zip_output_switch, settings.zip_output);
    zip_output_switch->property_active().signal_changed().connect([this] { s_signal_changed.emit(); });

    drill_mode_combo->set_active_id(GerberOutputSettings::mode_lut.lookup_reverse(settings.drill_mode));
    drill_mode_combo->signal_changed().connect([this] {
        settings.drill_mode =
                GerberOutputSettings::mode_lut.lookup(static_cast<std::string>(drill_mode_combo->get_active_id()));
        update_drill_visibility();
        s_signal_changed.emit();
        update_export_button();
    });
    update_drill_visibility();

    {
        Gtk::Button *blind_buried_update_button;
        GET_WIDGET(blind_buried_update_button);
        blind_buried_update_button->signal_clicked().connect(
                sigc::mem_fun(*this, &FabOutputWindow::update_blind_buried_drills));
    }

    generate_button->signal_clicked().connect(sigc::mem_fun(*this, &FabOutputWindow::generate));

    outline_width_sp = Gtk::manage(new SpinButtonDim());
    outline_width_sp->set_range(.01_mm, 10_mm);
    outline_width_sp->show();
    bind_widget(outline_width_sp, settings.outline_width);
    outline_width_sp->signal_value_changed().connect([this] { s_signal_changed.emit(); });
    {
        Gtk::Box *b = nullptr;
        x->get_widget("gerber_outline_width_box", b);
        b->pack_start(*outline_width_sp, true, true, 0);
    }

    sg_layer_name = Gtk::SizeGroup::create(Gtk::SIZE_GROUP_HORIZONTAL);
    sg_drill_span_name = Gtk::SizeGroup::create(Gtk::SIZE_GROUP_HORIZONTAL);

    odb_export_filechooser_filename.attach(odb_filename_entry, odb_filename_button, this);
    odb_export_filechooser_filename.set_project_dir(project_dir);
    odb_export_filechooser_filename.bind_filename(odb_settings.output_filename);
    odb_export_filechooser_filename.signal_changed().connect([this] {
        s_signal_changed.emit();
        update_export_button();
    });
    odb_export_filechooser_filename.set_action(GTK_FILE_CHOOSER_ACTION_SAVE);

    odb_export_filechooser_directory.attach(odb_directory_entry, odb_directory_button, this);
    odb_export_filechooser_directory.set_project_dir(project_dir);
    odb_export_filechooser_directory.bind_filename(odb_settings.output_directory);
    odb_export_filechooser_directory.signal_changed().connect([this] {
        s_signal_changed.emit();
        update_export_button();
    });
    odb_export_filechooser_directory.set_action(GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER);

    stack->set_visible_child(Board::output_format_lut.lookup_reverse(brd.output_format));
    stack->property_visible_child_name().signal_changed().connect([this] {
        brd.output_format = Board::output_format_lut.lookup(stack->get_visible_child_name());
        s_signal_changed.emit();
        update_export_button();
    });

    {
        std::map<ODBOutputSettings::Format, Gtk::RadioButton *> format_map = {
                {ODBOutputSettings::Format::DIRECTORY, odb_format_directory_rb},
                {ODBOutputSettings::Format::TGZ, odb_format_tgz_rb},
                {ODBOutputSettings::Format::ZIP, odb_format_zip_rb},
        };
        bind_widget<ODBOutputSettings::Format>(format_map, odb_settings.format, [this](auto v) {
            s_signal_changed.emit();
            update_odb_visibility();
            update_export_button();
        });
    }
    update_odb_visibility();

    bind_widget(odb_job_name_entry, odb_settings.job_name, [this](std::string &) { s_signal_changed.emit(); });

    reload_layers();
    reload_drills();
    update_export_button();
}

void FabOutputWindow::reload_layers()
{
    if (n_layers == settings.layers.size())
        return;
    else
        n_layers = settings.layers.size();


    blind_buried_box->set_visible(brd.get_n_inner_layers());

    {
        auto children = gerber_layers_box->get_children();
        for (auto ch : children)
            delete ch;
    }
    std::vector<GerberOutputSettings::GerberLayer *> layers_sorted;
    layers_sorted.reserve(settings.layers.size());
    for (auto &la : settings.layers) {
        layers_sorted.push_back(&la.second);
    }
    std::sort(layers_sorted.begin(), layers_sorted.end(),
              [](const auto a, const auto b) { return b->layer < a->layer; });

    for (auto la : layers_sorted) {
        auto ed = GerberLayerEditor::create(*this, *la);
        ed->signal_changed().connect([this] {
            s_signal_changed.emit();
            update_export_button();
        });
        gerber_layers_box->add(*ed);
        ed->show();
        ed->unreference();
    }
}

void FabOutputWindow::reload_drills()
{
    {
        auto children = blind_buried_drills_box->get_children();
        for (auto ch : children)
            delete ch;
    }

    for (auto &[span, filename] : settings.blind_buried_drills_filenames | ranges::views::reverse) {
        auto ed = DrillEditor::create(*this, brd.get_layer_name(span), filename);
        ed->signal_changed().connect([this] {
            s_signal_changed.emit();
            update_export_button();
        });
        blind_buried_drills_box->add(*ed);
        ed->show();
        ed->unreference();
    }
}

void FabOutputWindow::update_drill_visibility()
{
    if (settings.drill_mode == GerberOutputSettings::DrillMode::INDIVIDUAL) {
        npth_filename_entry->set_visible(true);
        npth_filename_label->set_visible(true);
        pth_filename_label->set_text("PTH suffix");
    }
    else {
        npth_filename_entry->set_visible(false);
        npth_filename_label->set_visible(false);
        pth_filename_label->set_text("Drill suffix");
    }
}

void FabOutputWindow::update_odb_visibility()
{
    const bool is_dir = odb_settings.format == ODBOutputSettings::Format::DIRECTORY;
    odb_directory_box->set_visible(is_dir);
    odb_directory_label->set_visible(is_dir);
    odb_filename_box->set_visible(!is_dir);
    odb_filename_label->set_visible(!is_dir);
}

void FabOutputWindow::update_blind_buried_drills()
{
    settings.update_blind_buried_drills_filenames(brd);
    reload_drills();
}

static void cb_nop(const std::string &)
{
}

void FabOutputWindow::generate()
{
    if (!generate_button->get_sensitive())
        return;

    RulesCheckCache cache(core);
    const auto r = rules_check(*core.get_rules(), RuleID::PREFLIGHT_CHECKS, core, cache, &cb_nop);
    const auto r_plane = rules_check(*core.get_rules(), RuleID::PLANE, core, cache, &cb_nop);
    if (r.level != RulesCheckErrorLevel::PASS || r_plane.level != RulesCheckErrorLevel::PASS) {
        Gtk::MessageDialog md(*this, "Preflight or plane checks didn't pass", false /* use_markup */,
                              Gtk::MESSAGE_ERROR, Gtk::BUTTONS_NONE);
        md.set_secondary_text("This might be due to unfilled planes or overlapping planes of identical priority.");
        md.add_button("Ignore", Gtk::RESPONSE_ACCEPT);
        md.add_button("Cancel", Gtk::RESPONSE_CANCEL);
        md.set_default_response(Gtk::RESPONSE_CANCEL);
        if (md.run() != Gtk::RESPONSE_ACCEPT) {
            return;
        }
    }

    if (brd.output_format == Board::OutputFormat::GERBER) {
        std::string error_str;
        try {
            GerberOutputSettings my_settings = settings;
            my_settings.output_directory = export_filechooser.get_filename_abs();
            my_settings.zip_output = zip_output_switch->get_active();
            GerberExporter ex(brd, my_settings);
            ex.generate();
            log_textview->get_buffer()->set_text(ex.get_log());
            done_revealer_controller.show_success("Gerber output done");
        }
        catch (const std::exception &e) {
            error_str = std::string("Error: ") + e.what();
        }
        catch (const Gio::Error &e) {
            error_str = std::string("Error: ") + e.what();
        }
        catch (...) {
            error_str = "Other error";
        }
        if (error_str.size()) {
            log_textview->get_buffer()->set_text(error_str);
            done_revealer_controller.show_error(error_str);
        }
    }
    else {
        try {
            ODBOutputSettings my_settings = odb_settings;
            my_settings.output_filename = odb_export_filechooser_filename.get_filename_abs();
            my_settings.output_directory = odb_export_filechooser_directory.get_filename_abs();
            export_odb(brd, my_settings);
            odb_done_revealer_controller.show_success("ODB++ output done");
        }
        catch (const std::exception &e) {
            odb_done_revealer_controller.show_error("Error: " + std::string(e.what()));
        }
        catch (...) {
            odb_done_revealer_controller.show_error("unknown error");
        }
    }
}

void FabOutputWindow::set_can_generate(bool v)
{
    can_export = v;
    update_export_button();
}

void FabOutputWindow::update_export_button()
{
    std::string txt;
    if (can_export) {
        if (brd.output_format == Board::OutputFormat::GERBER) {
            if (settings.output_directory.size()) {
                if (settings.prefix.size()) {
                    if (settings.drill_mode == GerberOutputSettings::DrillMode::INDIVIDUAL) {
                        if (settings.drill_pth_filename.size() == 0 || settings.drill_npth_filename.size() == 0) {
                            txt = "drill filenames not set";
                        }
                    }
                    else {
                        if (settings.drill_pth_filename.size() == 0) {
                            txt = "drill filename not set";
                        }
                    }
                    if (txt.size() == 0) { // still okay
                        for (const auto &it : settings.layers) {
                            if (it.second.enabled && it.second.filename.size() == 0) {
                                txt = brd.get_layers().at(it.first).name + " filename not set";
                                break;
                            }
                        }
                    }
                    if (txt.size() == 0) {
                        for (const auto &[span, filename] : settings.blind_buried_drills_filenames) {
                            if (filename.size() == 0) {
                                txt = brd.get_layer_name(span) + " drill filename not set";
                                break;
                            }
                        }
                    }
                }
                else {
                    txt = "prefix not set";
                }
            }
            else {
                txt = "output directory not set";
            }
        }
        else {
            switch (odb_settings.format) {
            case ODBOutputSettings::Format::DIRECTORY:
                if (odb_settings.output_directory.size() == 0) {
                    txt = "ODB++ directory not set";
                }
                break;

            case ODBOutputSettings::Format::ZIP:
            case ODBOutputSettings::Format::TGZ:
                if (odb_settings.output_filename.size() == 0) {
                    txt = "ODB++ filename not set";
                }
                break;
            }
        }
    }
    else {
        txt = "tool is active";
    }
    widget_set_insensitive_tooltip(*generate_button, txt);
}

FabOutputWindow *FabOutputWindow::create(Gtk::Window *p, IDocumentBoard &c, const std::string &project_dir)
{
    FabOutputWindow *w;
    Glib::RefPtr<Gtk::Builder> x = Gtk::Builder::create();
    x->add_from_resource("/org/horizon-eda/horizon/imp/fab_output.ui");
    x->get_widget_derived("window", w, c, project_dir);

    w->set_transient_for(*p);

    return w;
}
} // namespace horizon
