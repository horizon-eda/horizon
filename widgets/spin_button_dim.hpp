#pragma once
#include <gtkmm.h>

namespace horizon {
	class SpinButtonDim: public Gtk::SpinButton {
		public:
			SpinButtonDim();

		protected:
			 virtual int on_input(double* new_value);
			 virtual bool on_output();
	};
}
