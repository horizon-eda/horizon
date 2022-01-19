#pragma once
#include <gtkmm.h>
#include <memory>
#include <zmq.hpp>
#include <optional>

namespace horizon {

class PoolProjectManagerViewCreateProject {
public:
    PoolProjectManagerViewCreateProject(const Glib::RefPtr<Gtk::Builder> &refBuilder,
                                        class PoolProjectManagerAppWindow &w);
    void clear();
    void focus();
    std::optional<std::string> create();
    typedef sigc::signal<void, bool> type_signal_valid_change;
    type_signal_valid_change signal_valid_change()
    {
        return s_signal_valid_change;
    }
    void populate_pool_combo();
    void update();

private:
    PoolProjectManagerAppWindow &win;
    Gtk::FileChooserButton *project_path_chooser = nullptr;
    Gtk::Label *project_dir_label = nullptr;
    Gtk::ComboBoxText *project_pool_combo = nullptr;
    std::map<std::string, std::string> meta_values;
    class ProjectMetaEditor *meta_editor = nullptr;

    type_signal_valid_change s_signal_valid_change;
};

class PoolProjectManagerViewProject {
public:
    PoolProjectManagerViewProject(const Glib::RefPtr<Gtk::Builder> &refBuilder, class PoolProjectManagerAppWindow &w);
    Gtk::Label *label_project_title = nullptr;
    Gtk::Label *label_project_author = nullptr;
    Gtk::Label *label_project_directory = nullptr;

    void open_top_schematic();
    void open_board();
    bool update_meta();
    void update_pool_cache_status(const class PoolCacheStatus &status);
    void reset_pool_cache_status();

private:
    PoolProjectManagerAppWindow &win;
    Gtk::Button *button_top_schematic = nullptr;
    Gtk::Button *button_board = nullptr;
    Gtk::Button *button_part_browser = nullptr;
    Gtk::Button *button_project_pool = nullptr;
    Gtk::Label *pool_cache_status_label = nullptr;

    class OpeningSpinner *board_spinner = nullptr;
    class OpeningSpinner *schematic_spinner = nullptr;

    void handle_button_part_browser();
    void handle_button_project_pool();
};
} // namespace horizon
