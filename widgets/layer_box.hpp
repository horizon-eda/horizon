#pragma once
#include <gtkmm.h>
#include "core/core.hpp"
#include "sheet.hpp"
#include "canvas/layer_display.hpp"

namespace horizon {
	class LayerBox: public Gtk::Box {
		public:
			LayerBox(Core *c);

			void update();
			Glib::PropertyProxy<int> property_work_layer() { return p_property_work_layer.get_proxy(); }
			typedef sigc::signal<void, int, LayerDisplay> type_signal_set_layer_display;
			type_signal_set_layer_display signal_set_layer_display() {return s_signal_set_layer_display;}


			Glib::PropertyProxy<float> property_layer_opacity() { return p_property_layer_opacity.get_proxy(); }

			Glib::PropertyProxy<bool> property_select_work_layer_only() { return p_property_select_work_layer_only.get_proxy(); }
			json serialize();
			void load_from_json(const json &j);

		private:
			class ListColumns : public Gtk::TreeModelColumnRecord {
				public:
					ListColumns() {
						Gtk::TreeModelColumnRecord::add(name) ;
						Gtk::TreeModelColumnRecord::add(index) ;
						Gtk::TreeModelColumnRecord::add(visible) ;
						Gtk::TreeModelColumnRecord::add(color) ;
						Gtk::TreeModelColumnRecord::add(display_mode) ;
						Gtk::TreeModelColumnRecord::add(is_work) ;
					}
					Gtk::TreeModelColumn<Glib::ustring> name;
					Gtk::TreeModelColumn<int> index;
					Gtk::TreeModelColumn<bool> visible;
					Gtk::TreeModelColumn<bool> is_work;
					Gtk::TreeModelColumn<Gdk::RGBA> color;
					Gtk::TreeModelColumn<int> display_mode;
			} ;
			ListColumns list_columns;

			Core *core;

			Gtk::TreeView *view;
			Glib::RefPtr<Gtk::ListStore> store;
			void toggled(const Glib::ustring& path);
			void work_toggled(const Glib::ustring& path);
			void activated(const Glib::ustring& path);
			void selection_changed();
			Glib::Property<int> p_property_work_layer;
			Glib::Property<bool> p_property_select_work_layer_only;
			Glib::Property<float> p_property_layer_opacity;
			type_signal_set_layer_display s_signal_set_layer_display;
			void emit_layer_display(const Gtk::TreeModel::Row &row);
			void update_work_layer();

			Glib::RefPtr<Glib::Binding> binding_select_work_layer_only;
			Glib::RefPtr<Glib::Binding> binding_layer_opacity;
			void visible_cell_data_func(Gtk::CellRenderer *cr, const Gtk::TreeModel::iterator &it);



	};


}
