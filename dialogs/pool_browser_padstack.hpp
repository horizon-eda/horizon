#pragma once
#include <gtkmm.h>
#include <array>
#include <set>
#include "common.hpp"
#include "uuid.hpp"
#include "uuid_path.hpp"
#include "pool.hpp"
namespace horizon {


	class PoolBrowserDialogPadstack: public Gtk::Dialog {
		public:
		PoolBrowserDialogPadstack(Gtk::Window *parent, Pool *ipool, const UUID &ipackage_uuid);
			UUID selected_uuid;
			ObjectType selected_object_type;
			bool selection_valid = false;
			//virtual ~MainWindow();
		private :
			Pool *pool;
			UUID package_uuid;

			class ListColumns : public Gtk::TreeModelColumnRecord {
				public:
					ListColumns() {
						Gtk::TreeModelColumnRecord::add( padstack_name ) ;
						Gtk::TreeModelColumnRecord::add( uuid ) ;
					}
					Gtk::TreeModelColumn<Glib::ustring> padstack_name;
					Gtk::TreeModelColumn<UUID> uuid;
			} ;
			ListColumns list_columns;

			Gtk::TreeView *view;
			Glib::RefPtr<Gtk::ListStore> store;



			void ok_clicked();
			void row_activated(const Gtk::TreeModel::Path& path, Gtk::TreeViewColumn* column);
			bool auto_ok();

	};
}
