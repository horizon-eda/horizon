#pragma once
#include <gtkmm.h>
#include "util/uuid.hpp"

namespace horizon {
	class NetSelector: public Gtk::Box {
		public:
			NetSelector(class Block *b);
			void set_power_only(bool p);
			void set_bus_mode(bool b);
			void set_bus_member_mode(const UUID &bus_uuid);
			UUID get_selected_net();
			void select_net(const UUID &uu);

			typedef sigc::signal<void, UUID> type_signal_selected;
			//type_signal_selected signal_selected() {return s_signal_selected;}
			type_signal_selected signal_activated() {return s_signal_activated;}
			void update();

		private:
			class ListColumns : public Gtk::TreeModelColumnRecord {
				public:
					ListColumns() {
						Gtk::TreeModelColumnRecord::add( name ) ;
						Gtk::TreeModelColumnRecord::add( uuid ) ;
						Gtk::TreeModelColumnRecord::add( is_power ) ;
					}
					Gtk::TreeModelColumn<Glib::ustring> name;
					Gtk::TreeModelColumn<UUID> uuid;
					Gtk::TreeModelColumn<bool> is_power;
			} ;
			ListColumns list_columns;
			Block *block;
			bool power_only = false;
			bool bus_mode = false;
			bool bus_member_mode = false;
			class Bus *bus = nullptr;


			Gtk::TreeView *view;
			Glib::RefPtr<Gtk::ListStore> store;

			//type_signal_selected s_signal_selected;
			type_signal_selected s_signal_activated;
			void row_activated(const Gtk::TreeModel::Path& path, Gtk::TreeViewColumn* column);
	};


}
