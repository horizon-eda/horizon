#pragma once
#include <gtkmm.h>
#include "util/uuid.hpp"
#include "util/changeable.hpp"

namespace horizon {
class MultiNetSelector : public Gtk::Grid, public Changeable {
public:
    MultiNetSelector(const class Block &block);
    void select_nets(const std::set<UUID> &nets);
    std::set<UUID> get_selected_nets() const;

    void update();

private:
    const Block &block;

    std::set<UUID> nets_selected;

    class ListColumns : public Gtk::TreeModelColumnRecord {
    public:
        ListColumns()
        {
            Gtk::TreeModelColumnRecord::add(name);
            Gtk::TreeModelColumnRecord::add(uuid);
        }
        Gtk::TreeModelColumn<Glib::ustring> name;
        Gtk::TreeModelColumn<UUID> uuid;
    };
    ListColumns list_columns;

    Gtk::TreeView *view_available = nullptr;
    Gtk::TreeView *view_selected = nullptr;

    Gtk::Widget *make_listview(Gtk::TreeView *&view, Glib::RefPtr<Gtk::TreeModelSort> &store);

    Gtk::Button *button_include = nullptr;
    Gtk::Button *button_exclude = nullptr;

    Glib::RefPtr<Gtk::ListStore> store;
    Glib::RefPtr<Gtk::TreeModelFilter> filter_available;
    Glib::RefPtr<Gtk::TreeModelSort> sort_available;

    Glib::RefPtr<Gtk::TreeModelFilter> filter_selected;
    Glib::RefPtr<Gtk::TreeModelSort> sort_selected;

    Glib::RefPtr<Gtk::SizeGroup> sg_views;
};
} // namespace horizon
