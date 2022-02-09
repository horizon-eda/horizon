#pragma once
#include <gtkmm.h>
#include "util/uuid.hpp"
#include "util/changeable.hpp"
#include <set>

namespace horizon {
class MultiItemSelector : public Gtk::Grid, public Changeable {
public:
    MultiItemSelector();
    void select_items(const std::set<UUID> &nets);
    std::set<UUID> get_selected_items() const;

    virtual void update() = 0;

protected:
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

    Glib::RefPtr<Gtk::ListStore> store;

    virtual std::string get_column_heading() const = 0;
    void construct();

private:
    std::set<UUID> items_selected;


    Gtk::TreeView *view_available = nullptr;
    Gtk::TreeView *view_selected = nullptr;

    Gtk::Widget *make_listview(Gtk::TreeView *&view, Glib::RefPtr<Gtk::TreeModelSort> &store);

    Gtk::Button *button_include = nullptr;
    Gtk::Button *button_exclude = nullptr;

    Glib::RefPtr<Gtk::TreeModelFilter> filter_available;
    Glib::RefPtr<Gtk::TreeModelSort> sort_available;

    Glib::RefPtr<Gtk::TreeModelFilter> filter_selected;
    Glib::RefPtr<Gtk::TreeModelSort> sort_selected;

    Glib::RefPtr<Gtk::SizeGroup> sg_views;
};
} // namespace horizon
