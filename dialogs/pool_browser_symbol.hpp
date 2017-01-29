#pragma once
#include <gtkmm.h>
#include <array>
#include <set>
#include "common.hpp"
#include "uuid.hpp"
#include "uuid_path.hpp"
#include "pool.hpp"
#include "core/core.hpp"
namespace horizon {


	class PoolBrowserDialogSymbol: public Gtk::Dialog {
		public:
		PoolBrowserDialogSymbol(Gtk::Window *parent, Core *core, const UUID &iunit_uuid);
			UUID selected_uuid;
			ObjectType selected_object_type;
			bool selection_valid = false;
			//virtual ~MainWindow();
		private :
			Pool *pool;
			Core *core;
			UUID unit_uuid;

			class ListColumns : public Gtk::TreeModelColumnRecord {
				public:
					ListColumns() {
						Gtk::TreeModelColumnRecord::add( symbol_name ) ;
						Gtk::TreeModelColumnRecord::add( unit_name ) ;
						Gtk::TreeModelColumnRecord::add( uuid ) ;
					}
					Gtk::TreeModelColumn<Glib::ustring> unit_name;
					Gtk::TreeModelColumn<Glib::ustring> symbol_name;
					Gtk::TreeModelColumn<UUID> uuid;
			} ;
			ListColumns list_columns;

			Gtk::TreeView *view;
			class CanvasGL *canvas;
			Glib::RefPtr<Gtk::ListStore> store;



			void ok_clicked();
			void row_activated(const Gtk::TreeModel::Path& path, Gtk::TreeViewColumn* column);
			bool auto_ok();

	};
}
