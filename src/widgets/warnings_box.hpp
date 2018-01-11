#pragma once
#include <gtkmm.h>
#include "core/core.hpp"
#include "schematic/sheet.hpp"

namespace horizon {
	class WarningsBox: public Gtk::Box {
		public:
			WarningsBox();

			void update(const std::vector<Warning> &warnings);
			typedef sigc::signal<void, const Coordi&> type_signal_selected;
			type_signal_selected signal_selected() {return s_signal_selected;}

		private:
			class ListColumns : public Gtk::TreeModelColumnRecord {
				public:
					ListColumns() {
						Gtk::TreeModelColumnRecord::add( text ) ;
						Gtk::TreeModelColumnRecord::add( position ) ;
					}
					Gtk::TreeModelColumn<Glib::ustring> text;
					Gtk::TreeModelColumn<Coordi> position;
			} ;
			ListColumns list_columns;

			Gtk::TreeView *view;
			Glib::RefPtr<Gtk::ListStore> store;

			type_signal_selected s_signal_selected;
			void row_activated(const Gtk::TreeModel::Path& path, Gtk::TreeViewColumn* column);
	};


}
