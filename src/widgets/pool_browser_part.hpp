#pragma once
#include "pool_browser.hpp"

namespace horizon {
	class PoolBrowserPart: public PoolBrowser {
		public:
			PoolBrowserPart(class Pool *p, const UUID &euuid = UUID());
			void search() override;
			void set_MPN(const std::string &s);
			void set_entity_uuid(const UUID &uu);
			ObjectType get_type() const override {return ObjectType::PART;}

		protected:
			Glib::RefPtr<Gtk::ListStore> create_list_store() override;
			void create_columns() override;
			void add_sort_controller_columns() override;
			UUID uuid_from_row(const Gtk::TreeModel::Row &row) override;

		private:
			class ListColumns : public Gtk::TreeModelColumnRecord {
				public:
					ListColumns() {
						Gtk::TreeModelColumnRecord::add( MPN ) ;
						Gtk::TreeModelColumnRecord::add( manufacturer ) ;
						Gtk::TreeModelColumnRecord::add( description ) ;
						Gtk::TreeModelColumnRecord::add( package ) ;
						Gtk::TreeModelColumnRecord::add( uuid ) ;
						Gtk::TreeModelColumnRecord::add( tags) ;
						Gtk::TreeModelColumnRecord::add( path ) ;
					}
					Gtk::TreeModelColumn<Glib::ustring> MPN;
					Gtk::TreeModelColumn<Glib::ustring> manufacturer;
					Gtk::TreeModelColumn<Glib::ustring> description;
					Gtk::TreeModelColumn<Glib::ustring> package;
					Gtk::TreeModelColumn<Glib::ustring> tags;
					Gtk::TreeModelColumn<Glib::ustring> path;
					Gtk::TreeModelColumn<UUID> uuid;
			} ;
			ListColumns list_columns;
			Gtk::Entry *MPN_entry = nullptr;
			Gtk::Entry *manufacturer_entry = nullptr;
			Gtk::Entry *tags_entry = nullptr;
			UUID entity_uuid;
	};
}
