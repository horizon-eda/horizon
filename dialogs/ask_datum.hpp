#pragma once
#include <gtkmm.h>
#include <array>
#include <set>
#include "widgets/spin_button_dim.hpp"
namespace horizon {


	class AskDatumDialog: public Gtk::Dialog {
		public:
			AskDatumDialog(Gtk::Window *parent, const std::string &label, bool mode_xy = false);
			SpinButtonDim *sp = nullptr;
			SpinButtonDim *sp_x = nullptr;
			SpinButtonDim *sp_y = nullptr;
			Gtk::CheckButton *cb_x = nullptr;
			Gtk::CheckButton *cb_y = nullptr;

			//virtual ~MainWindow();
		private :



	};
}
