#pragma once
#include <gtkmm.h>
#include <array>
#include <set>
namespace horizon {

	class FabOutputWindow: public Gtk::Window {
		friend class GerberLayerEditor;
		public:
			FabOutputWindow(BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>& x, class Board *b, class FabOutputSettings *settings);
			static FabOutputWindow* create(Gtk::Window *p, class Board *b, class FabOutputSettings *settings);

			void set_can_generate(bool v);
		private :
			class Board *brd;
			class FabOutputSettings *settings;
			Gtk::ListBox *gerber_layers_box = nullptr;
			Gtk::Entry *npth_filename_entry = nullptr;
			Gtk::Entry *pth_filename_entry = nullptr;
			Gtk::Label *npth_filename_label = nullptr;
			Gtk::Label *pth_filename_label = nullptr;
			Gtk::Entry *prefix_entry = nullptr;
			Gtk::Entry *directory_entry = nullptr;
			class SpinButtonDim *outline_width_sp = nullptr;
			Gtk::Button *generate_button = nullptr;
			Gtk::Button *directory_button = nullptr;
			Gtk::ComboBoxText *drill_mode_combo = nullptr;
			Gtk::TextView *log_textview = nullptr;


			Glib::RefPtr<Gtk::SizeGroup> sg_layer_name;

			void generate();
			void update_drill_visibility();

	};
}
