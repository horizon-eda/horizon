#pragma once
#include <array>
#include <gtkmm.h>
#include <set>
#include "util/window_state_store.hpp"
#include "util/export_file_chooser.hpp"
#include "util/changeable.hpp"

namespace horizon {

class FabOutputWindow : public Gtk::Window, public Changeable {
    friend class GerberLayerEditor;

public:
    FabOutputWindow(BaseObjectType *cobject, const Glib::RefPtr<Gtk::Builder> &x, class IDocumentBoard *c,
                    const std::string &project_dir);
    static FabOutputWindow *create(Gtk::Window *p, class IDocumentBoard *c, const std::string &project_dir);

    void set_can_generate(bool v);
    void reload_layers();
    void generate();

private:
    class IDocumentBoard *core;
    class Board *brd;
    class FabOutputSettings *settings;
    Gtk::ListBox *gerber_layers_box = nullptr;
    Gtk::Entry *npth_filename_entry = nullptr;
    Gtk::Entry *pth_filename_entry = nullptr;
    Gtk::Label *npth_filename_label = nullptr;
    Gtk::Label *pth_filename_label = nullptr;
    Gtk::Entry *prefix_entry = nullptr;
    Gtk::Entry *directory_entry = nullptr;
    class SpinButtonDim *outline_width_sp = nullptr;
    Gtk::Button *generate_button = nullptr;
    Gtk::Button *directory_button = nullptr;
    Gtk::ComboBoxText *drill_mode_combo = nullptr;
    Gtk::TextView *log_textview = nullptr;
    Gtk::Switch *zip_output_switch = nullptr;
    bool can_export = true;
    void update_export_button();

    ExportFileChooser export_filechooser;

    Glib::RefPtr<Gtk::SizeGroup> sg_layer_name;

    WindowStateStore state_store;

    void update_drill_visibility();
    unsigned int n_layers = 0;
};
} // namespace horizon
