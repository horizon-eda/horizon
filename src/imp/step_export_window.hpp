#pragma once
#include <gtkmm.h>
#include <array>
#include <set>
#include "util/uuid.hpp"
#include "util/export_file_chooser.hpp"
#include "util/changeable.hpp"

namespace horizon {

class StepExportWindow : public Gtk::Window, public Changeable {
public:
    StepExportWindow(BaseObjectType *cobject, const Glib::RefPtr<Gtk::Builder> &x, class IDocumentBoard &c,
                     const std::string &project_dir);
    static StepExportWindow *create(Gtk::Window *p, class IDocumentBoard &c, const std::string &project_dir);

    void set_can_export(bool v);
    void generate();

private:
    class IDocumentBoard &core;
    class STEPExportSettings &settings;
    Gtk::HeaderBar *header = nullptr;
    Gtk::Entry *filename_entry = nullptr;
    Gtk::Button *filename_button = nullptr;
    Gtk::Button *export_button = nullptr;
    Gtk::Switch *include_3d_models_switch = nullptr;
    Gtk::Box *min_dia_box = nullptr;
    class SpinButtonDim *min_dia_spin_button = nullptr;
    Gtk::Entry *prefix_entry = nullptr;
    Gtk::Button *pkg_incl_button = nullptr;
    Gtk::Button *pkg_excl_button = nullptr;
    Gtk::TreeView *pkgs_included_tv = nullptr;
    Gtk::TreeView *pkgs_excluded_tv = nullptr;

    class ListColumns : public Gtk::TreeModelColumnRecord {
    public:
        ListColumns()
        {
            Gtk::TreeModelColumnRecord::add(pkg_name);
            Gtk::TreeModelColumnRecord::add(pkg_count);
            Gtk::TreeModelColumnRecord::add(uuid);
        }
        Gtk::TreeModelColumn<Glib::ustring> pkg_name;
        Gtk::TreeModelColumn<unsigned int> pkg_count;
        Gtk::TreeModelColumn<UUID> uuid;
    };
    ListColumns list_columns;

    Glib::RefPtr<Gtk::ListStore> pkgs_store;
    Glib::RefPtr<Gtk::TreeModelFilter> pkgs_included;
    Glib::RefPtr<Gtk::TreeModelFilter> pkgs_excluded;

    Gtk::TextView *log_textview = nullptr;
    Gtk::Spinner *spinner = nullptr;
    bool can_export = true;
    void update_export_button();
    Glib::RefPtr<Gtk::TextTag> tag;

    class MyExportFileChooser : public ExportFileChooser {
    protected:
        void prepare_chooser(Glib::RefPtr<Gtk::FileChooser> chooser) override;
        void prepare_filename(std::string &filename) override;
    };
    MyExportFileChooser export_filechooser;

    Glib::Dispatcher export_dispatcher;
    std::mutex msg_queue_mutex;
    std::deque<std::string> msg_queue;
    bool export_running = false;

    void pkgs_fill();
    void update_pkg_incl_excl_sensitivity();
    void pkg_incl_excl(bool incl);

    void set_is_busy(bool v);

    void export_thread(STEPExportSettings settings);
};
} // namespace horizon
