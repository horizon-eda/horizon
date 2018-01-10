#pragma once
#include <gtkmm.h>
#include <array>
#include <set>
namespace horizon {


	class SchematicPropertiesDialog: public Gtk::Dialog {
		public:
			SchematicPropertiesDialog(Gtk::Window *parent, class Schematic *c);


		private :
			class Schematic *sch;


			void ok_clicked();
	};
}
