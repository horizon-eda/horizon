#pragma once
#include <array>
#include <gtkmm.h>
#include <set>
#include "util/window_state_store.hpp"
#include "util/changeable.hpp"
#include "util/export_file_chooser.hpp"
#include "board/pnp.hpp"
#include "widgets/column_chooser.hpp"

namespace horizon {

class PnPExportWindow : public Gtk::Window, public Changeable {

public:
    PnPExportWindow(BaseObjectType *cobject, const Glib::RefPtr<Gtk::Builder> &x, const class Board &brd,
                    class PnPExportSettings &settings, const std::string &project_dir);
    static PnPExportWindow *create(Gtk::Window *p, const class Board &brd, class PnPExportSettings &settings,
                                   const std::string &project_dir);

    void set_can_export(bool v);
    void generate();
    void update_preview();
    void update();

private:
    const class Board &board;
    class PnPExportSettings &settings;

    ExportFileChooser export_filechooser;

    Gtk::Button *export_button = nullptr;
    Gtk::Label *done_label = nullptr;
    Gtk::Revealer *done_revealer = nullptr;
    Gtk::Entry *directory_entry = nullptr;
    Gtk::Button *directory_button = nullptr;

    Gtk::ComboBoxText *mode_combo = nullptr;
    Gtk::CheckButton *nopopulate_check = nullptr;
    Gtk::Label *filename_merged_label = nullptr;
    Gtk::Label *filename_top_label = nullptr;
    Gtk::Label *filename_bottom_label = nullptr;
    Gtk::Entry *filename_merged_entry = nullptr;
    Gtk::Entry *filename_top_entry = nullptr;
    Gtk::Entry *filename_bottom_entry = nullptr;
    bool can_export = true;
    void update_export_button();

    void update_filename_visibility();

    Gtk::TreeView *preview_tv = nullptr;

    WindowStateStore state_store;

    ColumnChooser *column_chooser = nullptr;

    class MyAdapter : public ColumnChooser::Adapter<PnPColumn> {
    public:
        using ColumnChooser::Adapter<PnPColumn>::Adapter;
        std::string get_column_name(int col) const override;
        std::map<int, std::string> get_column_names() const override;
    };

    MyAdapter adapter;

    void flash(const std::string &s);
    sigc::connection flash_connection;

    class ListColumnsPreview : public Gtk::TreeModelColumnRecord {
    public:
        ListColumnsPreview()
        {
            Gtk::TreeModelColumnRecord::add(row);
        }
        Gtk::TreeModelColumn<PnPRow> row;
    };
    ListColumnsPreview list_columns_preview;

    Glib::RefPtr<Gtk::ListStore> store;
};
} // namespace horizon
