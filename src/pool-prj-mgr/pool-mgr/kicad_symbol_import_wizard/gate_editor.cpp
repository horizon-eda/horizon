#include "gate_editor.hpp"
#include "../part_wizard/location_entry.hpp"
#include "util/util.hpp"
#include "util/str_util.hpp"
#include "kicad_symbol_import_wizard.hpp"
#include "pool-prj-mgr/pool-prj-mgr-app_win.hpp"
#include "pool/pool.hpp"
#include "pool-prj-mgr/pool-mgr/editors/editor_window.hpp"


namespace horizon {

GateEditorImportWizard::GateEditorImportWizard(BaseObjectType *cobject, const Glib::RefPtr<Gtk::Builder> &x,
                                               const UUID &g, const UUID &u, KiCadSymbolImportWizard &pa)
    : Gtk::Box(cobject), parent(pa), gate_uu(g), unit_uu(u)
{
    x->get_widget("gate_label", gate_label);
    x->get_widget("button_gate_edit_symbol", edit_symbol_button);
    x->get_widget("button_gate_edit_unit", edit_unit_button);
    edit_symbol_button->signal_clicked().connect(sigc::mem_fun(*this, &GateEditorImportWizard::handle_edit_symbol));
    edit_unit_button->signal_clicked().connect(sigc::mem_fun(*this, &GateEditorImportWizard::handle_edit_unit));

    {
        Gtk::Button *from_entity_button;
        unit_location_entry = parent.pack_location_entry(x, "gate_unit_location_box", &from_entity_button);
        from_entity_button->set_label("From entity");
        from_entity_button->signal_clicked().connect([this] {
            auto rel = get_suffixed_filename_from_entity();
            unit_location_entry->set_filename(Glib::build_filename(parent.pool.get_base_path(), "units", rel));
        });
        unit_location_entry->set_filename(Glib::build_filename(parent.pool.get_base_path(), "units"));
        unit_location_entry->signal_changed().connect(
                sigc::mem_fun(parent, &KiCadSymbolImportWizard::update_can_finish));
    }

    {
        Gtk::Button *from_entity_button;
        symbol_location_entry = parent.pack_location_entry(x, "gate_symbol_location_box", &from_entity_button);
        from_entity_button->set_label("From entity");
        from_entity_button->signal_clicked().connect([this] {
            auto rel = get_suffixed_filename_from_entity();
            symbol_location_entry->set_filename(Glib::build_filename(parent.pool.get_base_path(), "symbols", rel));
        });
        symbol_location_entry->set_filename(Glib::build_filename(parent.pool.get_base_path(), "symbols"));
        symbol_location_entry->signal_changed().connect(
                sigc::mem_fun(parent, &KiCadSymbolImportWizard::update_can_finish));
    }

    auto &gate = parent.pool.get_entity(parent.entity_uuid)->gates.at(gate_uu);
    gate_label->set_text("Gate " + gate.name);
}

std::string GateEditorImportWizard::get_suffixed_filename_from_entity()
{
    auto rel = parent.get_rel_entity_filename();
    auto &gate = parent.pool.get_entity(parent.entity_uuid)->gates.at(gate_uu);
    std::string suffix = gate.suffix;
    trim(suffix);
    if (suffix.size() && endswith(rel, ".json")) {
        rel.insert(rel.size() - 5, "-" + suffix);
    }
    return rel;
}

GateEditorImportWizard *GateEditorImportWizard::create(const UUID &g, const UUID &u, KiCadSymbolImportWizard &pa)
{
    GateEditorImportWizard *w;
    Glib::RefPtr<Gtk::Builder> x = Gtk::Builder::create();
    x->add_from_resource(
            "/org/horizon-eda/horizon/pool-prj-mgr/pool-mgr/kicad_symbol_import_wizard/kicad_symbol_import_wizard.ui");
    x->get_widget_derived("gate_editor", w, g, u, pa);
    w->reference();
    return w;
}


void GateEditorImportWizard::handle_edit_unit()
{
    std::string filename = parent.pool.get_tmp_filename(ObjectType::UNIT, unit_uu);
    auto proc = parent.appwin->spawn(PoolProjectManagerProcess::Type::UNIT, {filename});
    parent.processes.emplace(filename, proc);
    proc->signal_exited().connect([this, filename](int status, bool modified) {
        parent.processes.erase(filename);
        parent.update_can_finish();
    });
    parent.update_can_finish();
}

void GateEditorImportWizard::handle_edit_symbol()
{
    std::string filename = parent.pool.get_tmp_filename(ObjectType::SYMBOL, parent.symbols.at(unit_uu));
    auto proc = parent.appwin->spawn(PoolProjectManagerProcess::Type::IMP_SYMBOL, {filename});
    parent.processes.emplace(filename, proc);
    proc->signal_exited().connect([this, filename](int status, bool modified) {
        parent.processes.erase(filename);
        parent.update_can_finish();
    });
    parent.update_can_finish();
}

} // namespace horizon
