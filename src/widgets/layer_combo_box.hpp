#pragma once
#include <gtkmm.h>

namespace horizon {
class LayerComboBox : public Gtk::ComboBox {
public:
    LayerComboBox();

    void set_active_layer(int l);
    int get_active_layer() const;

    void remove_all();
    void prepend(const class Layer &l);
    void set_layer_insensitive(int layer);

private:
    class ListColumns : public Gtk::TreeModelColumnRecord {
    public:
        ListColumns()
        {
            Gtk::TreeModelColumnRecord::add(layer);
            Gtk::TreeModelColumnRecord::add(name);
            Gtk::TreeModelColumnRecord::add(color);
            Gtk::TreeModelColumnRecord::add(sensitive);
        }
        Gtk::TreeModelColumn<int> layer;
        Gtk::TreeModelColumn<Glib::ustring> name;
        Gtk::TreeModelColumn<Gdk::RGBA> color;
        Gtk::TreeModelColumn<bool> sensitive;
    };
    ListColumns list_columns;

    Glib::RefPtr<Gtk::ListStore> store;
};
} // namespace horizon
