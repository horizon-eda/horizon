#pragma once
#include <gtkmm.h>
#include "common/common.hpp"
#include <set>
#include "widgets/pin_names_editor.hpp"


namespace horizon {
class PadEditor : public Gtk::Box {
    friend class PartWizard;

public:
    PadEditor(BaseObjectType *cobject, const Glib::RefPtr<Gtk::Builder> &x, const class Pad *p, class PartWizard *pa);
    static PadEditor *create(const class Pad *p, PartWizard *pa);
    std::string get_gate_name();
    const std::set<const Pad *> get_pads() const
    {
        return pads;
    }

private:
    class PartWizard *parent;
    std::set<const Pad *> pads;
    std::vector<std::string> names;
    void update_names();

    PinNamesEditor::PinNames pin_names;

    Gtk::Label *pad_names_label = nullptr;
    Gtk::Entry *pin_name_entry = nullptr;
    PinNamesEditor *pin_names_editor = nullptr;
    Gtk::ComboBoxText *dir_combo = nullptr;
    Gtk::SpinButton *swap_group_spin_button = nullptr;
    Gtk::ComboBox *combo_gate = nullptr;
    Gtk::Entry *combo_gate_entry = nullptr;
};
} // namespace horizon
