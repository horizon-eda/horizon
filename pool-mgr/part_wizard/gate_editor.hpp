#pragma once
#include <gtkmm.h>
#include "common.hpp"
#include <set>
#include "uuid_ptr.hpp"
#include "gate.hpp"


namespace horizon {
	class GateEditorWizard: public Gtk::Box {
		friend class PartWizard;
		public:
			GateEditorWizard(BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>& x, Gate *g, class PartWizard *pa);
			static GateEditorWizard* create(Gate *g, PartWizard *pa);
			void load();

			virtual ~GateEditorWizard() {}
		private :
			PartWizard *parent;
			uuid_ptr<Gate> gate;

			Gtk::Label *gate_label = nullptr;
			Gtk::Button *edit_symbol_button = nullptr;
			class LocationEntry *unit_location_entry = nullptr;
			class LocationEntry *symbol_location_entry = nullptr;
			Gtk::Entry *unit_name_entry = nullptr;
			Gtk::Button *unit_name_from_mpn_button = nullptr;
			Gtk::Entry *suffix_entry = nullptr;
	};
}
