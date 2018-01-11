#pragma once
#include <gtkmm.h>
#include <array>
#include <set>
#include "common/common.hpp"
namespace horizon {
	class SymbolPinNamesDialog: public Gtk::Dialog {
		public:
			SymbolPinNamesDialog(Gtk::Window *parent, class SchematicSymbol *s);
			bool valid = false;


		private :
			class SchematicSymbol *sym = nullptr;


			void ok_clicked();
	};
}
