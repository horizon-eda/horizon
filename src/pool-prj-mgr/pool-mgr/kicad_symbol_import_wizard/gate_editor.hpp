#pragma once
#include <gtkmm.h>
#include "common/common.hpp"
#include <set>
#include "util/uuid.hpp"
#include "pool/entity.hpp"

namespace horizon {
class GateEditorImportWizard : public Gtk::Box {
    friend class KiCadSymbolImportWizard;

public:
    GateEditorImportWizard(BaseObjectType *cobject, const Glib::RefPtr<Gtk::Builder> &x, const UUID &g,
                           const UUID &unit_uu, class KiCadSymbolImportWizard &pa);
    static GateEditorImportWizard *create(const UUID &g, const UUID &unit_uu, KiCadSymbolImportWizard &pa);
    void handle_edit_symbol();
    void handle_edit_unit();


private:
    KiCadSymbolImportWizard &parent;
    const UUID gate_uu;
    const UUID unit_uu;

    Gtk::Label *gate_label = nullptr;
    Gtk::Button *edit_unit_button = nullptr;
    Gtk::Button *edit_symbol_button = nullptr;
    class LocationEntry *unit_location_entry = nullptr;
    class LocationEntry *symbol_location_entry = nullptr;
    std::string get_suffixed_filename_from_entity();
};
} // namespace horizon
