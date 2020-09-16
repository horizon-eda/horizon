#pragma once
#include <gtkmm.h>
#include "common/common.hpp"
#include "editor_interface.hpp"

namespace horizon {

class EntityEditor : public Gtk::Box, public PoolEditorInterface {
    friend class GateEditor;

public:
    EntityEditor(BaseObjectType *cobject, const Glib::RefPtr<Gtk::Builder> &x, class Entity &e, class IPool &p);
    static EntityEditor *create(class Entity &e, class IPool &p);
    void reload() override;

    virtual ~EntityEditor(){};

private:
    class Entity &entity;
    Gtk::Entry *name_entry = nullptr;
    Gtk::Entry *manufacturer_entry = nullptr;
    Gtk::Entry *prefix_entry = nullptr;
    class TagEntry *tag_entry = nullptr;

    Gtk::ListBox *gates_listbox = nullptr;
    Gtk::ToolButton *refresh_button = nullptr;
    Gtk::ToolButton *add_button = nullptr;
    Gtk::ToolButton *delete_button = nullptr;

    Glib::RefPtr<Gtk::SizeGroup> sg_name;
    Glib::RefPtr<Gtk::SizeGroup> sg_suffix;
    Glib::RefPtr<Gtk::SizeGroup> sg_swap_group;
    Glib::RefPtr<Gtk::SizeGroup> sg_unit;

    void handle_add();
    void handle_delete();
    IPool &pool;

    void bind_entry(Gtk::Entry *e, std::string &s);
};
} // namespace horizon
