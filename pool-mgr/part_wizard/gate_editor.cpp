#include "gate_editor.hpp"
#include "part_wizard.hpp"
#include "location_entry.hpp"

namespace horizon {

	GateEditorWizard::GateEditorWizard(BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>& x, Gate *g, PartWizard *pa) :
		Gtk::Box(cobject), parent(pa), gate (g) {
		x->get_widget("gate_label", gate_label);
		x->get_widget("gate_edit_symbol", edit_symbol_button);
		x->get_widget("gate_suffix", suffix_entry);
		x->get_widget("gate_unit_name", unit_name_entry);
		x->get_widget("gate_unit_name_from_mpn", unit_name_from_mpn_button);

		gate_label->set_text("Gate: "+gate->name);

		{
			Gtk::Button *from_part_button;
			unit_location_entry = PartWizard::pack_location_entry(x, "gate_unit_location_box", &from_part_button);
			from_part_button->set_label("From part");
			from_part_button->signal_clicked().connect([this] {
				auto part_fn = Gio::File::create_for_path(parent->part_location_entry->get_filename());
				auto part_base = Gio::File::create_for_path(Glib::build_filename(parent->pool_base_path, "parts"));
				auto rel = part_base->get_relative_path(part_fn);
				unit_location_entry->set_filename(Glib::build_filename(parent->pool_base_path, "units", rel));
			});
			unit_location_entry->set_filename(Glib::build_filename(parent->pool_base_path, "units"));
		}

		{
			Gtk::Button *from_part_button;
			symbol_location_entry = PartWizard::pack_location_entry(x, "gate_symbol_location_box", &from_part_button);
			from_part_button->set_label("From part");
			from_part_button->signal_clicked().connect([this] {
				auto part_fn = Gio::File::create_for_path(parent->part_location_entry->get_filename());
				auto part_base = Gio::File::create_for_path(Glib::build_filename(parent->pool_base_path, "parts"));
				auto rel = part_base->get_relative_path(part_fn);
				symbol_location_entry->set_filename(Glib::build_filename(parent->pool_base_path, "symbols", rel));
			});
			symbol_location_entry->set_filename(Glib::build_filename(parent->pool_base_path, "symbols"));
		}

		unit_name_from_mpn_button->signal_clicked().connect([this]{
			unit_name_entry->set_text(parent->part_mpn_entry->get_text());
		});
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
