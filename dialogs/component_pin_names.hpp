#pragma once
#include <gtkmm.h>
#include <array>
#include <set>
#include "common.hpp"
#include "uuid.hpp"
#include "component.hpp"
namespace horizon {


	class ComponentPinNamesDialog: public Gtk::Dialog {
		public:
			ComponentPinNamesDialog(Gtk::Window *parent, Component *c);
			bool valid = false;


		private :
			Component *comp = nullptr;


			void ok_clicked();
	};
}
