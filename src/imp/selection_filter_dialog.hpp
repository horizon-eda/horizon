#pragma once
#include <gtkmm.h>
#include "common/common.hpp"
#include "util/changeable.hpp"
#include <set>
namespace horizon {

class SelectionFilterDialog : public Gtk::Window, public Changeable {
public:
    SelectionFilterDialog(Gtk::Window *parent, class SelectionFilter &sf, class ImpBase &imp);
    void update_layers();
    bool get_filtered();
    void set_work_layer(int layer);

private:
    SelectionFilter &selection_filter;
    ImpBase &imp;
    Gtk::ListBox *listbox = nullptr;

    class Type {
    public:
        Gtk::ToggleButton *expand_button = nullptr;
        Gtk::CheckButton *checkbutton = nullptr;
        std::map<int, Gtk::CheckButton *> layer_buttons;
        Gtk::CheckButton *other_layer_checkbutton = nullptr;
        void update();
        bool get_all_active();
        bool expanded = false;
        bool blocked = false;
    };

    std::map<ObjectType, Type> checkbuttons;
    Gtk::Button *reset_button = nullptr;
    void update();
    void set_all(bool state);
    void connect_doubleclick(Gtk::CheckButton *cb);
    Gtk::CheckButton *add_layer_button(ObjectType type, int layer, int index, bool active = true);

    Gtk::CheckButton *work_layer_only_cb = nullptr;
    void update_work_layer_only();
    bool work_layer_only_before = false;
    bool work_layer_only = false;
    std::map<ObjectType, std::set<int>> saved;
    int work_layer = 0;
    void update_filter();
};
} // namespace horizon
