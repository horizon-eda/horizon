#pragma once
#include "project/project.hpp"
#include <gtkmm.h>
#include <memory>
#include <zmq.hpp>

namespace horizon {

class PoolProjectManagerViewCreateProject : public sigc::trackable {
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

private:
    PoolProjectManagerAppWindow *win = nullptr;
    Gtk::Entry *project_name_entry = nullptr;
    Gtk::Entry *project_description_entry = nullptr;
    Gtk::FileChooserButton *project_path_chooser = nullptr;
    Gtk::Label *project_dir_label = nullptr;
    Gtk::ComboBoxText *project_pool_combo = nullptr;
    void update();

    type_signal_valid_change s_signal_valid_change;
};

class PoolProjectManagerViewProject : public sigc::trackable {
public:
    PoolProjectManagerViewProject(const Glib::RefPtr<Gtk::Builder> &refBuilder, class PoolProjectManagerAppWindow *w);
    Gtk::Entry *entry_project_title = nullptr;
    Gtk::Label *label_pool_name = nullptr;
    Gtk::InfoBar *info_bar = nullptr;
    Gtk::Label *info_bar_label = nullptr;

private:
    PoolProjectManagerAppWindow *win = nullptr;
    Gtk::Button *button_top_schematic = nullptr;
    Gtk::Button *button_board = nullptr;
    Gtk::Button *button_part_browser = nullptr;
    Gtk::Button *button_pool_cache = nullptr;

    void handle_button_top_schematic();
    void handle_button_board();
    void handle_button_part_browser();
    void handle_button_pool_cache();
};
} // namespace horizon
