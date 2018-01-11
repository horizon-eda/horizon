#pragma once
#include <gtkmm.h>
#include <array>
#include <set>
#include "common/common.hpp"
#include "util/uuid.hpp"
namespace horizon {
	class EditStackupDialog: public Gtk::Dialog {
		friend class StackupLayerEditor;
		public:
			EditStackupDialog(Gtk::Window *parent, class Board *brd);

		private :
			class Board *board = nullptr;
			Gtk::ListBox *lb = nullptr;
			Gtk::SpinButton *sp_n_inner_layers = nullptr;
			void ok_clicked();
			void update_layers();
			std::map<std::pair<int, bool>, uint64_t> saved;
			Glib::RefPtr<Gtk::SizeGroup> sg_layer_name;
	};
}
