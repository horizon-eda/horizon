#pragma once
#include <gtkmm.h>
#include "common/common.hpp"
#include "pool/part.hpp"
#include "editor_interface.hpp"

namespace horizon {

class PartEditor : public Gtk::Box, public PoolEditorInterface {
public:
    PartEditor(BaseObjectType *cobject, const Glib::RefPtr<Gtk::Builder> &x, class Part *p, class Pool *po,
               class PoolParametric *pp);
    static PartEditor *create(class Part *p, class Pool *po, class PoolParametric *pp);
    void reload() override;
    void save() override;

    virtual ~PartEditor(){};

private:
    class Part *part;
    class Pool *pool;
    class PoolParametric *pool_parametric;

    class EntryWithInheritance *w_mpn = nullptr;
    class EntryWithInheritance *w_value = nullptr;
    class EntryWithInheritance *w_manufacturer = nullptr;
    class EntryWithInheritance *w_description = nullptr;
    class EntryWithInheritance *w_datasheet = nullptr;
    std::map<Part::Attribute, class EntryWithInheritance *> attr_editors;


    Gtk::Label *w_entity_label = nullptr;
    Gtk::Label *w_package_label = nullptr;
    Gtk::Label *w_base_label = nullptr;
    Gtk::Button *w_change_package_button = nullptr;
    Gtk::ComboBoxText *w_model_combo = nullptr;
    Gtk::ToggleButton *w_model_inherit = nullptr;

    class TagEntry *w_tags = nullptr;
    Gtk::Entry *w_tags_inherited = nullptr;
    Gtk::ToggleButton *w_tags_inherit = nullptr;

    Gtk::TreeView *w_tv_pins = nullptr;
    Gtk::TreeView *w_tv_pads = nullptr;
    Gtk::Button *w_button_map = nullptr;
    Gtk::Button *w_button_unmap = nullptr;
    Gtk::Button *w_button_automap = nullptr;
    Gtk::Button *w_button_select_pin = nullptr;
    Gtk::Button *w_button_select_pads = nullptr;
    Gtk::Button *w_button_copy_from_other = nullptr;
    Gtk::Label *w_pin_stat = nullptr;
    Gtk::Label *w_pad_stat = nullptr;

    Gtk::ComboBoxText *w_parametric_table_combo = nullptr;
    Gtk::Box *w_parametric_box = nullptr;
    Gtk::Button *w_parametric_from_base = nullptr;

    Gtk::Label *w_orderable_MPNs_label = nullptr;
    Gtk::Button *w_orderable_MPNs_add_button = nullptr;
    Gtk::Box *w_orderable_MPNs_box = nullptr;

    class PinListColumns : public Gtk::TreeModelColumnRecord {
    public:
        PinListColumns()
        {
            Gtk::TreeModelColumnRecord::add(gate_name);
            Gtk::TreeModelColumnRecord::add(gate_uuid);
            Gtk::TreeModelColumnRecord::add(pin_name);
            Gtk::TreeModelColumnRecord::add(pin_uuid);
            Gtk::TreeModelColumnRecord::add(mapped);
        }
        Gtk::TreeModelColumn<Glib::ustring> gate_name;
        Gtk::TreeModelColumn<Glib::ustring> pin_name;
        Gtk::TreeModelColumn<horizon::UUID> gate_uuid;
        Gtk::TreeModelColumn<horizon::UUID> pin_uuid;
        Gtk::TreeModelColumn<bool> mapped;
    };
    PinListColumns pin_list_columns;

    Glib::RefPtr<Gtk::ListStore> pin_store;

    class PadListColumns : public Gtk::TreeModelColumnRecord {
    public:
        PadListColumns()
        {
            Gtk::TreeModelColumnRecord::add(pad_name);
            Gtk::TreeModelColumnRecord::add(pad_uuid);
            Gtk::TreeModelColumnRecord::add(gate_name);
            Gtk::TreeModelColumnRecord::add(gate_uuid);
            Gtk::TreeModelColumnRecord::add(pin_name);
            Gtk::TreeModelColumnRecord::add(pin_uuid);
        }
        Gtk::TreeModelColumn<Glib::ustring> pad_name;
        Gtk::TreeModelColumn<horizon::UUID> pad_uuid;
        Gtk::TreeModelColumn<Glib::ustring> gate_name;
        Gtk::TreeModelColumn<Glib::ustring> pin_name;
        Gtk::TreeModelColumn<horizon::UUID> gate_uuid;
        Gtk::TreeModelColumn<horizon::UUID> pin_uuid;
    };
    PadListColumns pad_list_columns;

    Glib::RefPtr<Gtk::ListStore> pad_store;

    void update_orderable_MPNs_label();
    void update_treeview();
    void update_mapped();
    void update_entries();
    void change_package();
    void populate_models();
    void update_model_inherit();
    void map_pin(Gtk::TreeModel::iterator it_pin);
    void copy_from_other_part();
    void update_map_buttons();

    class ParametricEditor *parametric_editor = nullptr;
    void update_parametric_editor();
    class OrderableMPNEditor *create_orderable_MPN_editor(const UUID &uu);
};
} // namespace horizon
