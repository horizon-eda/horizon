#pragma once
#include <gtkmm.h>
#include <memory>
#include <set>
#include "util/uuid.hpp"
#include "util/sort_controller.hpp"
#include "util/selection_provider.hpp"
#include "common/common.hpp"

namespace horizon {
	class PoolBrowser: public Gtk::Box, public SelectionProvider {
		public:
			PoolBrowser(class Pool *pool);
			UUID get_selected() override;
			void set_show_none(bool v);
			void set_show_path(bool v);
			void add_context_menu_item(const std::string &label, sigc::slot1<void, UUID> cb);
			virtual void search() = 0;
			virtual ObjectType get_type() const {return ObjectType::INVALID;};
			void go_to(const UUID &uu);
			void clear_search();

		protected:
			void construct();
			class Pool *pool = nullptr;
			bool show_none = false;
			bool show_path = false;
			int path_column = -1;


			Gtk::TreeView *treeview = nullptr;

			Gtk::Entry *create_search_entry(const std::string &label);
			void add_search_widget(const std::string &label, Gtk::Widget &w);



			virtual Glib::RefPtr<Gtk::ListStore> create_list_store() = 0;
			virtual void create_columns() = 0;
			virtual void add_sort_controller_columns() = 0;
			virtual UUID uuid_from_row(const Gtk::TreeModel::Row &row) = 0;

			Glib::RefPtr<Gtk::ListStore> store;
			std::unique_ptr<SortController> sort_controller;

			void row_activated(const Gtk::TreeModel::Path& path, Gtk::TreeViewColumn* column);
			void selection_changed();

			void select_uuid(const UUID &uu);
			void scroll_to_selection();

			Gtk::Menu context_menu;
			std::set<Gtk::Entry*> search_entries;

		private :
			Gtk::Grid *grid = nullptr;
			int grid_top = 0;
	};
}
