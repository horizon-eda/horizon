#pragma once
#include <gtkmm.h>
#include "common/common.hpp"
#include <set>
#include "util/uuid_ptr.hpp"
#include "pool/gate.hpp"


namespace horizon {
class GateEditorWizard : public Gtk::Box {
    friend class PartWizard;

public:
    GateEditorWizard(BaseObjectType *cobject, const Glib::RefPtr<Gtk::Builder> &x, Gate *g, class PartWizard *pa);
    static GateEditorWizard *create(Gate *g, PartWizard *pa);
    void update_symbol_pins(unsigned int n_mapped);
    void set_can_edit_symbol_name(bool v);

    virtual ~GateEditorWizard()
    {
    }

private:
    PartWizard *parent;
    uuid_ptr<Gate> gate;

    Gtk::Label *gate_label = nullptr;
    Gtk::Label *gate_symbol_label = nullptr;
    Gtk::Button *edit_symbol_button = nullptr;
    class LocationEntry *unit_location_entry = nullptr;
    class LocationEntry *symbol_location_entry = nullptr;
    Gtk::Entry *unit_name_entry = nullptr;
    Gtk::Button *unit_name_from_mpn_button = nullptr;
    Gtk::Entry *symbol_name_entry = nullptr;
    Gtk::Button *symbol_name_from_unit_button = nullptr;
    Gtk::Entry *suffix_entry = nullptr;

    std::string get_suffixed_filename_from_part();
};
} // namespace horizon
