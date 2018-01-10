#pragma once
#include <gtkmm.h>

namespace horizon {
	class KeySequenceDialog: public Gtk::Dialog {
		public:
			KeySequenceDialog(Gtk::Window *parent);
			void add_sequence(const std::vector<unsigned int> &seq, const std::string &label);
			void add_sequence(const std::string &seq, const std::string &label);

		private :
			Gtk::ListBox *lb;
			Glib::RefPtr<Gtk::SizeGroup> sg;
	};


}
