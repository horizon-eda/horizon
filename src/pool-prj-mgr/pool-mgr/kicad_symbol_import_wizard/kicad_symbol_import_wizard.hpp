#pragma once
#include <gtkmm.h>
#include "common/common.hpp"
#include "pool/unit.hpp"
#include "pool/part.hpp"
#include "pool/entity.hpp"
#include "pool/pool.hpp"
#include "../pool_notebook.hpp" //for processes
#include "util/window_state_store.hpp"
#include "util/kicad_lib_parser.hpp"

namespace horizon {

class KiCadSymbolImportWizard : public Gtk::Window {
    friend class GateEditorImportWizard;

public:
    KiCadSymbolImportWizard(BaseObjectType *cobject, const Glib::RefPtr<Gtk::Builder> &x, const UUID &pkg_uuid,
                            class Pool &po, class PoolProjectManagerAppWindow *aw, const std::string &lib_filename);
    static KiCadSymbolImportWizard *create(const UUID &pkg_uuid, class Pool &po, class PoolProjectManagerAppWindow *aw,
                                           const std::string &lib_filename);
    std::vector<std::string> get_files_saved() const;
    void reload();

    ~KiCadSymbolImportWizard();

private:
    Pool &pool;

    class PoolProjectManagerAppWindow *appwin;

    Gtk::HeaderBar *header = nullptr;
    Gtk::Button *button_skip = nullptr;
    Gtk::Button *button_next = nullptr;
    Gtk::Button *button_finish = nullptr;
    Gtk::Button *button_prev = nullptr;
    Gtk::Box *edit_gates_box = nullptr;
    Gtk::Stack *stack = nullptr;
    class PoolBrowserPackage *browser_package = nullptr;
    class PreviewCanvas *symbol_preview = nullptr;
    Gtk::SpinButton *preview_part_sp = nullptr;
    Gtk::TreeView *tv_symbols = nullptr;
    Gtk::Box *part_box = nullptr;
    Gtk::CheckButton *merge_pins_cb = nullptr;
    Gtk::Label *fp_info_label = nullptr;
    Gtk::Label *fp_info_label_sym = nullptr;
    static std::string get_fp_info(const KiCadSymbol &s);

    void update_symbol_preview();
    void update_symbol_preview_part();
    std::vector<Symbol> symbols_for_preview;

    std::list<KiCadSymbol> k_symbols;
    const KiCadSymbol *k_sym = nullptr;
    void select_symbol();
    void import(const Package *pkg);

    Gtk::Button *button_part_edit = nullptr;
    Gtk::Button *button_autofill = nullptr;

    void autofill();

    class ListColumns : public Gtk::TreeModelColumnRecord {
    public:
        ListColumns()
        {
            Gtk::TreeModelColumnRecord::add(name);
            Gtk::TreeModelColumnRecord::add(sym);
        }
        Gtk::TreeModelColumn<Glib::ustring> name;
        Gtk::TreeModelColumn<const KiCadSymbol *> sym;
    };
    ListColumns list_columns;

    Glib::RefPtr<Gtk::ListStore> symbols_store;

    UUID entity_uuid;
    UUID part_uuid;
    std::map<UUID, UUID> symbols; // unit->symbol

    void handle_next();
    void handle_skip();
    void handle_finish();
    void handle_select();
    void handle_prev();
    void handle_edit_part();
    void handle_edit_entity();
    void finish();
    void update_can_finish();

    std::vector<std::string> get_filenames();
    std::vector<std::string> files_saved;


    class LocationEntry *entity_location_entry = nullptr;
    class LocationEntry *part_location_entry = nullptr;
    class LocationEntry *pack_location_entry(const Glib::RefPtr<Gtk::Builder> &x, const std::string &w,
                                             Gtk::Button **button_other = nullptr);

    std::map<std::string, class PoolProjectManagerProcess *> processes;

    void update_buttons();
    std::string get_rel_entity_filename();

    enum class Mode { SYMBOL, PACKAGE, EDIT };
    Mode mode = Mode::SYMBOL;
    void set_mode(Mode mo);

    WindowStateStore state_store;
};
} // namespace horizon
