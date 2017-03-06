#pragma once
#include <gtkmm.h>
#include "uuid.hpp"
#include "sort_controller.hpp"
#include "part_provider.hpp"

namespace horizon {
	class PoolBrowserPart: public Gtk::Box, public PartProvider {
		public:
			static PoolBrowserPart* create(class Pool *p, const UUID &e_uuid = UUID());
			PoolBrowserPart(BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>& x, Pool *pool, const UUID &e_uuid);

			UUID get_selected_part() override;
			void set_MPN(const std::string &s);
			void set_show_none(bool v);


		private:
			Pool *pool = nullptr;
			UUID entity_uuid;
			bool show_none = false;

			Gtk::TreeView *w_treeview = nullptr;
			Gtk::Entry *w_MPN_entry = nullptr;
			Gtk::Entry *w_manufacturer_entry = nullptr;
			Gtk::Entry *w_tags_entry = nullptr;

			class ListColumns : public Gtk::TreeModelColumnRecord {
				public:
					ListColumns() {
						Gtk::TreeModelColumnRecord::add( MPN ) ;
						Gtk::TreeModelColumnRecord::add( manufacturer ) ;
						Gtk::TreeModelColumnRecord::add( package ) ;
						Gtk::TreeModelColumnRecord::add( uuid ) ;
						Gtk::TreeModelColumnRecord::add( tags) ;
					}
					Gtk::TreeModelColumn<Glib::ustring> MPN;
					Gtk::TreeModelColumn<Glib::ustring> manufacturer;
					Gtk::TreeModelColumn<Glib::ustring> package;
					Gtk::TreeModelColumn<Glib::ustring> tags;
					Gtk::TreeModelColumn<UUID> uuid;
			} ;
			ListColumns list_columns;

			Glib::RefPtr<Gtk::ListStore> store;
			std::unique_ptr<SortController> sort_controller;

			void ok_clicked();
			void row_activated(const Gtk::TreeModel::Path& path, Gtk::TreeViewColumn* column);
			void selection_changed();
			bool auto_ok();
			void search();

		private :
	};
}
