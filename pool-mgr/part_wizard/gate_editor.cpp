#include "gate_editor.hpp"
#include "part_wizard.hpp"
#include "location_entry.hpp"

namespace horizon {

	static auto pack_location_entry(const Glib::RefPtr<Gtk::Builder>& x, const std::string &w) {
		auto en = Gtk::manage(new LocationEntry());
		Gtk::Box *box;
		x->get_widget(w, box);
		box->pack_start(*en, true, true, 0);
		en->show();
		return en;
	}

	GateEditorWizard::GateEditorWizard(BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>& x, Gate *g, PartWizard *pa) :
		Gtk::Box(cobject), parent(pa), gate (g) {
		x->get_widget("gate_label", gate_label);
		x->get_widget("gate_edit_symbol", edit_symbol_button);
		x->get_widget("gate_suffix", suffix_entry);
		x->get_widget("gate_unit_name", unit_name_entry);

		gate_label->set_text("Gate: "+gate->name);

		unit_location_entry = pack_location_entry(x, "gate_unit_location_box");
		unit_location_entry->set_filename(Glib::build_filename(parent->pool_base_path, "units"));

		symbol_location_entry = pack_location_entry(x, "gate_symbol_location_box");
		symbol_location_entry->set_filename(Glib::build_filename(parent->pool_base_path, "symbols"));
	}



	GateEditorWizard* GateEditorWizard::create(Gate *g, PartWizard *pa) {
		GateEditorWizard* w;
		Glib::RefPtr<Gtk::Builder> x = Gtk::Builder::create();
		x->add_from_resource("/net/carrotIndustries/horizon/pool-mgr/part_wizard/part_wizard.ui");
		std::cout << "create gate ed" << std::endl;
		x->get_widget_derived("gate_editor", w, g, pa);
		w->reference();
		return w;
	}
}
