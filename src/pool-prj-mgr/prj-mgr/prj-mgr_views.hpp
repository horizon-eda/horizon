#pragma once
#include "project/project.hpp"
#include <gtkmm.h>
#include <memory>
#include <zmq.hpp>

namespace horizon {

class PoolProjectManagerViewCreateProject {
public:
    PoolProjectManagerViewCreateProject(const Glib::RefPtr<Gtk::Builder> &refBuilder,
                                        class PoolProjectManagerAppWindow *w);
    void clear();
    std::pair<bool, std::string> create();
    typedef sigc::signal<void, bool> type_signal_valid_change;
    type_signal_valid_change signal_valid_change()
    {
        return s_signal_valid_change;
    }
    void populate_pool_combo(const Glib::RefPtr<Gtk::Application> &app);
    void update();

private:
    PoolProjectManagerAppWindow *win = nullptr;
    Gtk::Entry *project_name_entry = nullptr;
    Gtk::Entry *project_description_entry = nullptr;
    Gtk::FileChooserButton *project_path_chooser = nullptr;
    Gtk::Label *project_dir_label = nullptr;
    Gtk::ComboBoxText *project_pool_combo = nullptr;


    type_signal_valid_change s_signal_valid_change;
};

class PoolProjectManagerViewProject {
public:
    PoolProjectManagerViewProject(const Glib::RefPtr<Gtk::Builder> &refBuilder, class PoolProjectManagerAppWindow *w);
    Gtk::Entry *entry_project_title = nullptr;
    Gtk::Label *label_pool_name = nullptr;
    Gtk::Label *label_pool_path = nullptr;
    Gtk::Label *label_project_directory = nullptr;
    Gtk::InfoBar *pool_info_bar = nullptr;
    Gtk::Label *pool_info_bar_label = nullptr;

    void open_top_schematic();
    void open_board();

private:
    PoolProjectManagerAppWindow *win = nullptr;
    Gtk::Button *button_top_schematic = nullptr;
    Gtk::Button *button_board = nullptr;
    Gtk::Button *button_part_browser = nullptr;
    Gtk::Button *button_pool_cache = nullptr;
    Gtk::Button *button_change_pool = nullptr;

    void handle_button_part_browser();
    void handle_button_pool_cache();
    void handle_button_change_pool();
};
} // namespace horizon
