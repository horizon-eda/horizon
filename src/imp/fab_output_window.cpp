#include "fab_output_window.hpp"
#include "board/board.hpp"
#include "export_gerber/gerber_export.hpp"
#include "util/gtk_util.hpp"
#include "widgets/spin_button_dim.hpp"
#include "document/idocument_board.hpp"
#include "rules/rules_with_core.hpp"
#include "rules/cache.hpp"

namespace horizon {

class GerberLayerEditor : public Gtk::Box, public Changeable {
public:
    GerberLayerEditor(BaseObjectType *cobject, const Glib::RefPtr<Gtk::Builder> &x, FabOutputWindow *pa,
                      FabOutputSettings::GerberLayer *la);
    static GerberLayerEditor *create(FabOutputWindow *pa, FabOutputSettings::GerberLayer *la);
    FabOutputWindow *parent;

private:
    Gtk::CheckButton *gerber_layer_checkbutton = nullptr;
    Gtk::Entry *gerber_layer_filename_entry = nullptr;

    FabOutputSettings::GerberLayer *layer = nullptr;
};

GerberLayerEditor::GerberLayerEditor(BaseObjectType *cobject, const Glib::RefPtr<Gtk::Builder> &x, FabOutputWindow *pa,
                                     FabOutputSettings::GerberLayer *la)
    : Gtk::Box(cobject), parent(pa), layer(la)
{
    x->get_widget("gerber_layer_checkbutton", gerber_layer_checkbutton);
    x->get_widget("gerber_layer_filename_entry", gerber_layer_filename_entry);
    parent->sg_layer_name->add_widget(*gerber_layer_checkbutton);

    gerber_layer_checkbutton->set_label(parent->brd->get_layers().at(layer->layer).name);
    bind_widget(gerber_layer_checkbutton, layer->enabled);
    gerber_layer_checkbutton->signal_toggled().connect([this] { s_signal_changed.emit(); });
    bind_widget(gerber_layer_filename_entry, layer->filename);
    gerber_layer_filename_entry->signal_changed().connect([this] { s_signal_changed.emit(); });
}

GerberLayerEditor *GerberLayerEditor::create(FabOutputWindow *pa, FabOutputSettings::GerberLayer *la)
{
    GerberLayerEditor *w;
    Glib::RefPtr<Gtk::Builder> x = Gtk::Builder::create();
    x->add_from_resource("/org/horizon-eda/horizon/imp/fab_output.ui", "gerber_layer_editor");
    x->get_widget_derived("gerber_layer_editor", w, pa, la);
    w->reference();
    return w;
}

FabOutputWindow::FabOutputWindow(BaseObjectType *cobject, const Glib::RefPtr<Gtk::Builder> &x, IDocumentBoard *c,
                                 const std::string &project_dir)
    : Gtk::Window(cobject), core(c), brd(core->get_board()), settings(core->get_fab_output_settings()),
      state_store(this, "imp-fab-output")
{
    x->get_widget("gerber_layers_box", gerber_layers_box);
    x->get_widget("prefix_entry", prefix_entry);
    x->get_widget("directory_entry", directory_entry);
    x->get_widget("npth_filename_entry", npth_filename_entry);
    x->get_widget("pth_filename_entry", pth_filename_entry);
    x->get_widget("npth_filename_label", npth_filename_label);
    x->get_widget("pth_filename_label", pth_filename_label);
    x->get_widget("generate_button", generate_button);
    x->get_widget("directory_button", directory_button);
    x->get_widget("drill_mode_combo", drill_mode_combo);
    x->get_widget("log_textview", log_textview);
    x->get_widget("zip_output", zip_output_switch);

    export_filechooser.attach(directory_entry, directory_button, this);
    export_filechooser.set_project_dir(project_dir);
    export_filechooser.bind_filename(settings->output_directory);
    export_filechooser.signal_changed().connect([this] {
        s_signal_changed.emit();
        update_export_button();
    });
    export_filechooser.set_action(GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER);

    bind_widget(prefix_entry, settings->prefix, [this](std::string &) {
        s_signal_changed.emit();
        update_export_button();
    });
    prefix_entry->signal_changed().connect([this] { s_signal_changed.emit(); });
    bind_widget(npth_filename_entry, settings->drill_npth_filename, [this](std::string &) {
        s_signal_changed.emit();
        update_export_button();
    });
    bind_widget(pth_filename_entry, settings->drill_pth_filename, [this](std::string &) {
        s_signal_changed.emit();
        update_export_button();
    });
    bind_widget(zip_output_switch, settings->zip_output);
    zip_output_switch->property_active().signal_changed().connect([this] { s_signal_changed.emit(); });

    drill_mode_combo->set_active_id(FabOutputSettings::mode_lut.lookup_reverse(settings->drill_mode));
    drill_mode_combo->signal_changed().connect([this] {
        settings->drill_mode = FabOutputSettings::mode_lut.lookup(drill_mode_combo->get_active_id());
        update_drill_visibility();
        s_signal_changed.emit();
        update_export_button();
    });
    update_drill_visibility();

    generate_button->signal_clicked().connect(sigc::mem_fun(*this, &FabOutputWindow::generate));

    outline_width_sp = Gtk::manage(new SpinButtonDim());
    outline_width_sp->set_range(.01_mm, 10_mm);
    outline_width_sp->show();
    bind_widget(outline_width_sp, settings->outline_width);
    outline_width_sp->signal_value_changed().connect([this] { s_signal_changed.emit(); });
    {
        Gtk::Box *b = nullptr;
        x->get_widget("gerber_outline_width_box", b);
        b->pack_start(*outline_width_sp, true, true, 0);
    }

    sg_layer_name = Gtk::SizeGroup::create(Gtk::SIZE_GROUP_HORIZONTAL);

    reload_layers();
    update_export_button();
}

void FabOutputWindow::reload_layers()
{
    if (n_layers == settings->layers.size())
        return;
    else
        n_layers = settings->layers.size();

    {
        auto children = gerber_layers_box->get_children();
        for (auto ch : children)
            delete ch;
    }
    std::vector<FabOutputSettings::GerberLayer *> layers_sorted;
    layers_sorted.reserve(settings->layers.size());
    for (auto &la : settings->layers) {
        layers_sorted.push_back(&la.second);
    }
    std::sort(layers_sorted.begin(), layers_sorted.end(),
              [](const auto a, const auto b) { return b->layer < a->layer; });

    for (auto la : layers_sorted) {
        auto ed = GerberLayerEditor::create(this, la);
        ed->signal_changed().connect([this] {
            s_signal_changed.emit();
            update_export_button();
        });
        gerber_layers_box->add(*ed);
        ed->show();
        ed->unreference();
    }
}

void FabOutputWindow::update_drill_visibility()
{
    if (settings->drill_mode == FabOutputSettings::DrillMode::INDIVIDUAL) {
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

static void cb_nop(const std::string &)
{
}

void FabOutputWindow::generate()
{
    if (!generate_button->get_sensitive())
        return;

    RulesCheckCache cache(core);
    auto r = rules_check(core->get_rules(), RuleID::PREFLIGHT_CHECKS, core, cache, &cb_nop);
    if (r.level != RulesCheckErrorLevel::PASS) {
        Gtk::MessageDialog md(*this, "Preflight checks didn't pass", false /* use_markup */, Gtk::MESSAGE_ERROR,
                              Gtk::BUTTONS_NONE);
        md.set_secondary_text("This might be due to unfilled planes.");
        md.add_button("Ignore", Gtk::RESPONSE_ACCEPT);
        md.add_button("Cancel", Gtk::RESPONSE_CANCEL);
        md.set_default_response(Gtk::RESPONSE_CANCEL);
        if (md.run() != Gtk::RESPONSE_ACCEPT) {
            return;
        }
    }

    try {
        FabOutputSettings my_settings = *settings;
        my_settings.output_directory = export_filechooser.get_filename_abs();
        my_settings.zip_output = zip_output_switch->get_active();
        GerberExporter ex(brd, &my_settings);
        ex.generate();
        log_textview->get_buffer()->set_text(ex.get_log());
    }
    catch (const std::exception &e) {
        log_textview->get_buffer()->set_text(std::string("Error: ") + e.what());
    }
    catch (const Gio::Error &e) {
        log_textview->get_buffer()->set_text(std::string("Error: ") + e.what());
    }
    catch (...) {
        log_textview->get_buffer()->set_text("Other error");
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
        if (settings->output_directory.size()) {
            if (settings->prefix.size()) {
                if (settings->drill_mode == FabOutputSettings::DrillMode::INDIVIDUAL) {
                    if (settings->drill_pth_filename.size() == 0 || settings->drill_npth_filename.size() == 0) {
                        txt = "drill filenames not set";
                    }
                }
                else {
                    if (settings->drill_pth_filename.size() == 0) {
                        txt = "drill filename not set";
                    }
                }
                if (txt.size() == 0) { // still okay
                    for (const auto &it : settings->layers) {
                        if (it.second.enabled && it.second.filename.size() == 0) {
                            txt = brd->get_layers().at(it.first).name + " filename not set";
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
        txt = "tool is active";
    }
    widget_set_insensitive_tooltip(*generate_button, txt);
}

FabOutputWindow *FabOutputWindow::create(Gtk::Window *p, IDocumentBoard *c, const std::string &project_dir)
{
    FabOutputWindow *w;
    Glib::RefPtr<Gtk::Builder> x = Gtk::Builder::create();
    x->add_from_resource("/org/horizon-eda/horizon/imp/fab_output.ui");
    x->get_widget_derived("window", w, c, project_dir);

    w->set_transient_for(*p);

    return w;
}
} // namespace horizon
