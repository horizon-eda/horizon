#pragma once
#include <gtkmm.h>
#include <array>
#include <set>
namespace horizon {


	class AskDatumDialog: public Gtk::Dialog {
		public:
			AskDatumDialog(Gtk::Window *parent, const std::string &label, bool mode_xy = false);
			class SpinButtonDim *sp = nullptr;
			class SpinButtonDim *sp_x = nullptr;
			class SpinButtonDim *sp_y = nullptr;
			Gtk::CheckButton *cb_x = nullptr;
			Gtk::CheckButton *cb_y = nullptr;

			//virtual ~MainWindow();
		private :



	};
}
