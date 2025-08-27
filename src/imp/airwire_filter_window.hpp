#pragma once
#include <gtkmm.h>
#include "util/uuid.hpp"
#include "util/changeable.hpp"
#include "common/common.hpp"
#include <set>
#include <nlohmann/json.hpp>

namespace horizon {
using json = nlohmann::json;

class AirwireFilterWindow : public Gtk::Window, public Changeable {
public:
    static AirwireFilterWindow *create(Gtk::Window *p, const class Board &b);
    AirwireFilterWindow(BaseObjectType *cobject, const Glib::RefPtr<Gtk::Builder> &x, const class Board &b);

    void update_from_board();
    void update_nets();
    bool airwire_is_visible(const UUID &net) const;
    bool get_filtered() const;
    void set_all(bool v);
    void set_only(const std::set<UUID> &nets);

    typedef sigc::signal<void, std::set<UUID>> type_signal_selection_changed;
    type_signal_selection_changed signal_selection_changed()
    {
        return s_signal_selection_changed;
    }
    const std::map<UUID, ColorI> &get_net_colors() const
    {
        return net_colors;
    }

    json serialize();
    void load_from_json(const json &j);

private:
    const class Board &brd;
    const class Block &block;
    std::map<UUID, bool> airwires_visible;
    std::map<UUID, ColorI> net_colors;

    class ListColumns : public Gtk::TreeModelColumnRecord {
    public:
        ListColumns()
        {
            Gtk::TreeModelColumnRecord::add(net);
            Gtk::TreeModelColumnRecord::add(net_name);
            Gtk::TreeModelColumnRecord::add(net_class);
            Gtk::TreeModelColumnRecord::add(net_class_name);
            Gtk::TreeModelColumnRecord::add(airwires_visible);
            Gtk::TreeModelColumnRecord::add(n_airwires);
            Gtk::TreeModelColumnRecord::add(color);
        }
        Gtk::TreeModelColumn<UUID> net;
        Gtk::TreeModelColumn<Glib::ustring> net_name;
        Gtk::TreeModelColumn<UUID> net_class;
        Gtk::TreeModelColumn<Glib::ustring> net_class_name;
        Gtk::TreeModelColumn<bool> airwires_visible;
        Gtk::TreeModelColumn<unsigned int> n_airwires;
        Gtk::TreeModelColumn<Gdk::RGBA> color;
    };
    ListColumns list_columns;

    Glib::RefPtr<Gtk::ListStore> store;
    Glib::RefPtr<Gtk::TreeModelFilter> store_filtered;
    Glib::RefPtr<Gtk::TreeModelSort> store_sorted;
    Gtk::TreeView *treeview = nullptr;
    Gtk::Button *all_on_button = nullptr;
    Gtk::Button *all_off_button = nullptr;

    Gtk::ToggleButton *search_button = nullptr;
    Gtk::ToggleButton *airwires_button = nullptr;
    Gtk::Revealer *search_revealer = nullptr;
    Gtk::Revealer *airwires_revealer = nullptr;
    Gtk::ComboBoxText *netclass_combo = nullptr;
    UUID netclass_filter;

    Gtk::SearchEntry *search_entry = nullptr;
    std::optional<Glib::PatternSpec> search_spec;

    Gtk::CheckButton *airwires_only_cb = nullptr;

    Gtk::Menu context_menu;
    enum class MenuOP { CHECK, UNCHECK, TOGGLE, SET_COLOR, CLEAR_COLOR };
    void append_context_menu_item(const std::string &name, MenuOP op);

    type_signal_selection_changed s_signal_selection_changed;
};

} // namespace horizon
