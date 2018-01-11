#pragma once
#include <gtkmm.h>
#include <array>
#include <set>
#include "common/common.hpp"
#include "util/uuid.hpp"
#include "pool/unit.hpp"

namespace horizon {


	class MapPinDialog: public Gtk::Dialog {
		public:
			MapPinDialog(Gtk::Window *parent, const std::vector<std::pair<const Pin*, bool>> &pins);
			UUID selected_uuid;
			bool selection_valid = false;
			//virtual ~MainWindow();
		private :
			class ListColumns : public Gtk::TreeModelColumnRecord {
				public:
					ListColumns() {
						Gtk::TreeModelColumnRecord::add( name ) ;
						Gtk::TreeModelColumnRecord::add( uuid ) ;
					}
					Gtk::TreeModelColumn<Glib::ustring> name;
					Gtk::TreeModelColumn<UUID> uuid;
			} ;
			ListColumns list_columns;

			Gtk::TreeView *view;
			Glib::RefPtr<Gtk::ListStore> store;

			void ok_clicked();
			void row_activated(const Gtk::TreeModel::Path& path, Gtk::TreeViewColumn* column);

	};
}
