#pragma once
#include <gtkmm.h>
#include "common.hpp"
#include <set>


namespace horizon {
	class PadEditor: public Gtk::Box {
		friend class PartWizard;
		public:
			PadEditor(BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>& x, const class Pad *p, class PartWizard *pa);
			static PadEditor* create(const class Pad *p, PartWizard *pa);
			std::string get_gate_name();



		private :
			class PartWizard *parent;
			std::set<const Pad *> pads;
			std::vector<std::string> names;
			void update_names();

			Gtk::Label *pad_names_label = nullptr;
			Gtk::Entry *pin_name_entry = nullptr;
			Gtk::Entry *pin_names_entry = nullptr;
			Gtk::ComboBoxText *dir_combo = nullptr;
			Gtk::SpinButton *swap_group_spin_button = nullptr;
			Gtk::ComboBox *combo_gate = nullptr;
			Gtk::Entry *combo_gate_entry = nullptr;

	};
}
