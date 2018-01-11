#pragma once
#include <gtkmm.h>
#include <array>
#include <set>
#include "common/common.hpp"
#include "util/uuid.hpp"
#include "canvas/selection_filter.hpp"
#include "core/core.hpp"
namespace horizon {


	class SelectionFilterDialog: public Gtk::Window {
		public:
			SelectionFilterDialog(Gtk::Window *parent, SelectionFilter *sf, Core *c);
		private :

			SelectionFilter *selection_filter;
			Core *core;
			Gtk::ListBox *listbox = nullptr;
			std::vector<Gtk::CheckButton*> checkbuttons;
	};
}
