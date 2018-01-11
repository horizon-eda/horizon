#pragma once
#include <gtkmm.h>
#include <array>
#include <set>
#include "common/common.hpp"
#include "util/uuid.hpp"
namespace horizon {

	class ShapeDialog: public Gtk::Dialog {
		public:
			ShapeDialog(Gtk::Window *parent, class Shape *sh);
			bool valid = false;


		private :
			class Shape *shape= nullptr;
			Gtk::ComboBoxText *w_form = nullptr;
			std::vector<std::pair<Gtk::Widget*, Gtk::Widget*>> widgets;
			Gtk::Grid *grid;
			void update();
			class SpinButtonDim *add_dim(const std::string &text, int top);

			void ok_clicked();
	};
}
