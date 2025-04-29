#pragma once
#include <gtkmm.h>
#include "common/common.hpp"
#include "pool/part.hpp"
#include "editor_base.hpp"
#include "widgets/generic_combo_box.hpp"
#include "pool/part.hpp"

namespace horizon {

class PartEditor : public Gtk::Box, public PoolEditorBase {
public:
    PartEditor(BaseObjectType *cobject, const Glib::RefPtr<Gtk::Builder> &x, const std::string &filename,
               class IPool &po, class PoolParametric &pp);
    static PartEditor *create(const std::string &filename, class IPool &po, class PoolParametric &pp);
    void reload() override;

    void save_as(const std::string &fn) override;
    std::string get_name() const override;
    const UUID &get_uuid() const override;
    RulesCheckResult run_checks() const override;
    const FileVersion &get_version() const override;
    unsigned int get_required_version() const override;
    ObjectType get_type() const override;

    virtual ~PartEditor() {};

private:
    Part part;
    class PoolParametric &pool_parametric;
    void load();

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
    Gtk::MenuItem *w_set_base_menu_item = nullptr;
    Gtk::MenuItem *w_clear_base_menu_item = nullptr;
    Gtk::MenuItem *w_create_base_menu_item = nullptr;
    GenericComboBox<UUID> *w_model_combo = nullptr;
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

    Gtk::Label *w_orderable_MPNs_label = nullptr;
    Gtk::Button *w_orderable_MPNs_add_button = nullptr;
    Gtk::Box *w_orderable_MPNs_box = nullptr;

    Gtk::Label *w_flags_label = nullptr;
    Gtk::Grid *w_flags_grid = nullptr;

    Gtk::RadioButton *w_override_prefix_inherit_button = nullptr;
    Gtk::RadioButton *w_override_prefix_no_button = nullptr;
    Gtk::RadioButton *w_override_prefix_yes_button = nullptr;
    Gtk::Entry *w_override_prefix_entry = nullptr;
    sigc::connection override_prefix_entry_connection;

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
    void update_pad_map();

    void update_orderable_MPNs_label();
    void update_treeview();
    void update_mapped();
    void update_entries();
    void change_package();
    void set_package(std::shared_ptr<const Package> pkg);
    void set_base(std::shared_ptr<const Part> part);
    bool check_base(const UUID &new_base_uuid);
    void change_base();
    void clear_base();
    void create_base();
    void populate_models();
    void update_model_inherit();
    void map_pin(Gtk::TreeModel::iterator it_pin);
    void copy_from_other_part();
    void update_map_buttons();
    void update_flags_label();
    void update_prefix_entry();

    std::map<Part::OverridePrefix, Gtk::RadioButton *> override_prefix_radio_buttons;

    std::map<Part::Flag, class FlagEditor *> flag_editors;
    class ParametricEditor *parametric_editor = nullptr;
    void update_parametric_editor();
    std::map<std::string, std::map<std::string, std::string>> parametric_data;
    Glib::RefPtr<Gtk::SizeGroup> sg_parametric_label;

    class OrderableMPNEditor *create_orderable_MPN_editor(const UUID &uu);

    UUID pending_base_part;

    std::unique_ptr<HistoryManager::HistoryItem> make_history_item(const std::string &comment) override;
    void history_load(const HistoryManager::HistoryItem &it) override;
};
} // namespace horizon
