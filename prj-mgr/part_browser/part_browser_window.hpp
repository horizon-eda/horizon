#pragma once
#include <gtkmm.h>
#include <array>
#include <set>
#include "common.hpp"
#include "uuid.hpp"
#include "pool.hpp"
#include "part.hpp"

namespace horizon {

	class PartBrowserWindow: public Gtk::Window{
		public:
			PartBrowserWindow(BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>& x, const std::string &pool_path, std::deque<UUID> &favs);
			static PartBrowserWindow* create(Gtk::Window *p, const std::string &pool_path, std::deque<UUID> &favs);
			typedef sigc::signal<void, UUID> type_signal_place_part;
			type_signal_place_part signal_place_part() {return s_signal_place_part;}
			void placed_part(const UUID &uu);

		private :
			Gtk::Button *add_search_button = nullptr;
			Gtk::Notebook *notebook = nullptr;
			Gtk::Button *place_part_button = nullptr;
			Gtk::ToggleButton *fav_button = nullptr;
			Gtk::ListBox *lb_favorites = nullptr;
			Gtk::ListBox *lb_recent = nullptr;
			void handle_add_search();
			void handle_switch_page(Gtk::Widget *w, guint index);
			void handle_fav_toggled();
			void update_favorites();
			void update_recents();
			void handle_favorites_selected(Gtk::ListBoxRow *row);
			void handle_favorites_activated(Gtk::ListBoxRow *row);
			void handle_place_part();
			sigc::connection fav_toggled_conn;
			std::set<Gtk::Widget*> search_views;
			Pool pool;
			UUID part_current;
			void update_part_current();
			std::deque<UUID> &favorites;
			std::deque<UUID> recents;

			type_signal_place_part s_signal_place_part;

	};
}
