#pragma once
#include <gtkmm.h>
#include "common/common.hpp"
#include "pool/unit.hpp"
#include "pool/part.hpp"
#include "pool/entity.hpp"
#include "../pool_notebook.hpp" //for processes
#include "util/window_state_store.hpp"

namespace CSV {
class Csv;
}

namespace horizon {

class PartWizard : public Gtk::Window {
    friend class PadEditor;
    friend class GateEditorWizard;

public:
    PartWizard(BaseObjectType *cobject, const Glib::RefPtr<Gtk::Builder> &x, const UUID &pkg_uuid,
               const std::string &bp, class Pool *po, class PoolProjectManagerAppWindow *aw);
    static PartWizard *create(const UUID &pkg_uuid, const std::string &pool_base_path, class Pool *po,
                              class PoolProjectManagerAppWindow *aw);
    std::vector<std::string> get_files_saved() const;
    void reload();

    ~PartWizard();

private:
    const class Package *pkg = nullptr;
    void set_pkg(const Package *p);
    std::string pool_base_path;
    class Pool *pool;

    Gtk::HeaderBar *header = nullptr;
    Gtk::Button *button_back = nullptr;
    Gtk::Button *button_next = nullptr;
    Gtk::Button *button_finish = nullptr;
    Gtk::Button *button_select = nullptr;
    Gtk::Stack *stack = nullptr;
    class PoolBrowserPackage *browser_package = nullptr;

    Gtk::ListBox *pads_lb = nullptr;
    Gtk::ToolButton *button_link_pads = nullptr;
    Gtk::ToolButton *button_unlink_pads = nullptr;
    Gtk::ToolButton *button_import_pads = nullptr;

    Glib::RefPtr<Gtk::SizeGroup> sg_name;
    Gtk::Box *page_assign = nullptr;
    Gtk::Box *page_edit = nullptr;
    Gtk::Box *edit_left_box = nullptr;

    Gtk::Entry *entity_name_entry = nullptr;
    Gtk::Button *entity_name_from_mpn_button = nullptr;
    Gtk::Entry *entity_prefix_entry = nullptr;
    class TagEntry *entity_tags_entry = nullptr;

    Gtk::Entry *part_mpn_entry = nullptr;
    Gtk::Entry *part_value_entry = nullptr;
    Gtk::Entry *part_manufacturer_entry = nullptr;
    Gtk::Entry *part_datasheet_entry = nullptr;
    Gtk::Entry *part_description_entry = nullptr;
    class TagEntry *part_tags_entry = nullptr;
    Gtk::Button *part_autofill_button = nullptr;

    class LocationEntry *entity_location_entry = nullptr;
    class LocationEntry *part_location_entry = nullptr;

    Gtk::Grid *steps_grid = nullptr;

    Part part;
    Entity entity;

    class ListColumns : public Gtk::TreeModelColumnRecord {
    public:
        ListColumns()
        {
            Gtk::TreeModelColumnRecord::add(name);
        }
        Gtk::TreeModelColumn<Glib::ustring> name;
    };
    ListColumns list_columns;

    Glib::RefPtr<Gtk::ListStore> gate_name_store;
    void update_gate_names();
    void update_pin_warnings();
    std::map<std::pair<std::string, std::string>, std::set<class PadEditor *>> get_pin_names();
    void handle_link();
    void handle_unlink();
    void handle_import();
    void update_part();

    class PadImportItem {
    public:
        std::string pin;
        std::string gate = "Main";
        std::vector<std::string> alt;
        Pin::Direction direction = Pin::Direction::INPUT;
    };
    void import_pads(const json &j);
    void import_pads(CSV::Csv &csv);
    void import_pads(const std::map<std::string, PadImportItem> &items);

    void create_pad_editors();
    void autolink_pads();
    void link_pads(const std::deque<class PadEditor *> &eds);
    bool frozen = false;

    enum class Mode { PACKAGE, ASSIGN, EDIT };

    void handle_next();
    void handle_back();
    void handle_finish();
    void handle_select();
    void finish();

    std::string get_rel_part_filename();
    void update_can_finish();
    void autofill();
    void update_steps();
    bool valid = false;
    bool mpn_valid = false;
    bool part_filename_valid = false;
    bool gates_valid = false;
    std::vector<std::string> get_filenames();
    std::vector<std::string> files_saved;

    Mode mode = Mode::ASSIGN;
    void set_mode(Mode mo);
    void prepare_edit();
    std::map<std::string, Unit> units;
    std::map<UUID, UUID> symbols;                    // unit UUID -> symbol UUID
    std::map<UUID, unsigned int> symbol_pins_mapped; // unit UUID -> pins mapped
    void update_symbol_pins_mapped();

    std::map<std::string, class PoolProjectManagerProcess *> processes;
    std::set<UUID> symbols_open;

    PoolProjectManagerAppWindow *appwin;

    class LocationEntry *pack_location_entry(const Glib::RefPtr<Gtk::Builder> &x, const std::string &w,
                                             Gtk::Button **button_other = nullptr);

    WindowStateStore state_store;
};
} // namespace horizon
