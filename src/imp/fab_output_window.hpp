#pragma once
#include <array>
#include <gtkmm.h>
#include <set>
#include "util/window_state_store.hpp"
#include "util/export_file_chooser.hpp"
#include "util/changeable.hpp"
#include "util/done_revealer_controller.hpp"

namespace horizon {

class FabOutputWindow : public Gtk::Window, public Changeable {
    friend class GerberLayerEditor;
    friend class DrillEditor;

public:
    FabOutputWindow(BaseObjectType *cobject, const Glib::RefPtr<Gtk::Builder> &x, class IDocumentBoard &c,
                    const std::string &project_dir);
    static FabOutputWindow *create(Gtk::Window *p, class IDocumentBoard &c, const std::string &project_dir);

    void set_can_generate(bool v);
    void reload_layers();
    void generate();

private:
    class IDocumentBoard &core;
    class Board &brd;
    class GerberOutputSettings &settings;
    class ODBOutputSettings &odb_settings;
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

    Gtk::Box *blind_buried_box = nullptr;
    Gtk::ListBox *blind_buried_drills_box = nullptr;

    Gtk::Entry *odb_filename_entry = nullptr;
    Gtk::Button *odb_filename_button = nullptr;
    Gtk::Box *odb_filename_box = nullptr;
    Gtk::Label *odb_filename_label = nullptr;

    Gtk::Entry *odb_directory_entry = nullptr;
    Gtk::Button *odb_directory_button = nullptr;
    Gtk::Box *odb_directory_box = nullptr;
    Gtk::Label *odb_directory_label = nullptr;

    Gtk::RadioButton *odb_format_tgz_rb = nullptr;
    Gtk::RadioButton *odb_format_directory_rb = nullptr;
    Gtk::RadioButton *odb_format_zip_rb = nullptr;

    Gtk::Entry *odb_job_name_entry = nullptr;

    Gtk::Stack *stack = nullptr;

    ExportFileChooser export_filechooser;
    class ODBExportFileChooserFilename : public ExportFileChooser {
    public:
        ODBExportFileChooserFilename(const ODBOutputSettings &settings);

    protected:
        void prepare_chooser(Glib::RefPtr<Gtk::FileChooser> chooser) override;
        void prepare_filename(std::string &filename) override;

        const ODBOutputSettings &settings;
    };
    ODBExportFileChooserFilename odb_export_filechooser_filename;

    ExportFileChooser odb_export_filechooser_directory;

    Gtk::Revealer *done_revealer = nullptr;
    Gtk::Label *done_label = nullptr;
    Gtk::Button *done_close_button = nullptr;
    DoneRevealerController done_revealer_controller;

    Gtk::Revealer *odb_done_revealer = nullptr;
    Gtk::Label *odb_done_label = nullptr;
    Gtk::Button *odb_done_close_button = nullptr;
    DoneRevealerController odb_done_revealer_controller;

    Glib::RefPtr<Gtk::SizeGroup> sg_layer_name;
    Glib::RefPtr<Gtk::SizeGroup> sg_drill_span_name;

    WindowStateStore state_store;

    void update_drill_visibility();
    void update_odb_visibility();

    void update_blind_buried_drills();
    void reload_drills();

    unsigned int n_layers = 0;
};
} // namespace horizon
