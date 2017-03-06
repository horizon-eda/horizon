#include "part_browser_window.hpp"
#include "util.hpp"
#include "widgets/pool_browser_part.hpp"

namespace horizon {

	static void header_fun(Gtk::ListBoxRow *row, Gtk::ListBoxRow *before) {
		if (before && !row->get_header())	{
			auto ret = Gtk::manage(new Gtk::Separator);
			row->set_header(*ret);
		}
	}

	class UUIDBox: public Gtk::Box {
		public:
			using Gtk::Box::Box;
			UUID uuid;
	};

	PartBrowserWindow::PartBrowserWindow(BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>& x, const std::string &pool_path, std::deque<UUID> &favs) :
		Gtk::Window(cobject),
		pool(pool_path),
		favorites(favs){
		x->get_widget("notebook", notebook);
		x->get_widget("add_search_button", add_search_button);
		x->get_widget("place_part_button", place_part_button);
		x->get_widget("fav_button", fav_button);
		x->get_widget("lb_favorites", lb_favorites);
		x->get_widget("lb_recent", lb_recent);

		lb_favorites->set_header_func(sigc::ptr_fun(header_fun));
		lb_recent->set_header_func(sigc::ptr_fun(header_fun));
		lb_favorites->signal_row_selected().connect(sigc::mem_fun(this, &PartBrowserWindow::handle_favorites_selected));
		lb_favorites->signal_row_activated().connect(sigc::mem_fun(this, &PartBrowserWindow::handle_favorites_activated));
		lb_recent->signal_row_selected().connect(sigc::mem_fun(this, &PartBrowserWindow::handle_favorites_selected));
		lb_recent->signal_row_activated().connect(sigc::mem_fun(this, &PartBrowserWindow::handle_favorites_activated));

		add_search_button->signal_clicked().connect(sigc::mem_fun(this, &PartBrowserWindow::handle_add_search));
		notebook->signal_switch_page().connect(sigc::mem_fun(this, &PartBrowserWindow::handle_switch_page));
		fav_toggled_conn = fav_button->signal_toggled().connect(sigc::mem_fun(this, &PartBrowserWindow::handle_fav_toggled));
		place_part_button->signal_clicked().connect(sigc::mem_fun(this, &PartBrowserWindow::handle_place_part));

		update_part_current();
		update_favorites();

		handle_add_search();
	}

	void PartBrowserWindow::handle_favorites_selected(Gtk::ListBoxRow *row) {
		update_part_current();
	}

	void PartBrowserWindow::handle_favorites_activated(Gtk::ListBoxRow *row) {
		handle_place_part();
	}

	void PartBrowserWindow::handle_place_part()  {
		if(part_current)
			s_signal_place_part.emit(part_current);
	}

	void PartBrowserWindow::handle_fav_toggled() {
		std::cout << "fav toggled" << std::endl;
		if(part_current) {
			if(fav_button->get_active()) {
				assert(std::count(favorites.begin(), favorites.end(), part_current)==0);
				favorites.push_front(part_current);
			}
			else {
				assert(std::count(favorites.begin(), favorites.end(), part_current)==1);
				auto it = std::find(favorites.begin(), favorites.end(), part_current);
				favorites.erase(it);
			}
			update_favorites();
		}
	}

	void PartBrowserWindow::handle_switch_page(Gtk::Widget *w, guint index) {
		update_part_current();
	}

	void PartBrowserWindow::placed_part(const UUID &uu) {
		auto ncount = std::count(recents.begin(), recents.end(), uu);
		assert(ncount<2);
		if(ncount) {
			auto it = std::find(recents.begin(), recents.end(), uu);
			recents.erase(it);
		}
		recents.push_front(uu);
		update_recents();
	}

	void PartBrowserWindow::update_favorites() {
		auto children = lb_favorites->get_children();
		for(auto it: children) {
			lb_favorites->remove(*it);
		}

		for(const auto &it: favorites) {
			auto part = pool.get_part(it);
			if(part) {
				auto box = Gtk::manage(new UUIDBox(Gtk::ORIENTATION_VERTICAL, 4));
				box->uuid = it;
				auto la_MPN = Gtk::manage(new Gtk::Label());
				la_MPN->set_xalign(0);
				la_MPN->set_markup("<b>"+part->get_MPN()+"</b>");
				box->pack_start(*la_MPN, false, false, 0);

				auto la_mfr = Gtk::manage(new Gtk::Label());
				la_mfr->set_xalign(0);
				la_mfr->set_text(part->get_manufacturer());
				box->pack_start(*la_mfr, false, false, 0);

				box->set_margin_top(4);
				box->set_margin_bottom(4);
				box->set_margin_start(4);
				box->set_margin_end(4);
				lb_favorites->append(*box);
				box->show_all();
			}
		}
	}

	void PartBrowserWindow::update_recents() {
		auto children = lb_recent->get_children();
		for(auto it: children) {
			lb_recent->remove(*it);
		}

		for(const auto &it: recents) {
			auto part = pool.get_part(it);
			if(part) {
				auto box = Gtk::manage(new UUIDBox(Gtk::ORIENTATION_VERTICAL, 4));
				box->uuid = it;
				auto la_MPN = Gtk::manage(new Gtk::Label());
				la_MPN->set_xalign(0);
				la_MPN->set_markup("<b>"+part->get_MPN()+"</b>");
				box->pack_start(*la_MPN, false, false, 0);

				auto la_mfr = Gtk::manage(new Gtk::Label());
				la_mfr->set_xalign(0);
				la_mfr->set_text(part->get_manufacturer());
				box->pack_start(*la_mfr, false, false, 0);

				box->set_margin_top(4);
				box->set_margin_bottom(4);
				box->set_margin_start(4);
				box->set_margin_end(4);
				lb_recent->append(*box);
				box->show_all();
			}
		}
	}

	void PartBrowserWindow::update_part_current() {
		auto page = notebook->get_nth_page(notebook->get_current_page());
		PartProvider *prv = nullptr;
		prv = dynamic_cast<PoolBrowserPart*>(page);

		if(prv) {
			part_current = prv->get_selected_part();
		}
		else {
			if(page->get_name()== "fav") {
				auto row = lb_favorites->get_selected_row();
				if(row) {
					part_current = dynamic_cast<UUIDBox*>(row->get_child())->uuid;
				}
				else {
					part_current = UUID();
				}
			}
			else if(page->get_name()== "recent") {
				auto row = lb_recent->get_selected_row();
				if(row) {
					part_current = dynamic_cast<UUIDBox*>(row->get_child())->uuid;
				}
				else {
					part_current = UUID();
				}
			}
			else {
				part_current = UUID();
			}

		}
		auto ncount = std::count(favorites.begin(), favorites.end(), part_current);
		assert(ncount<2);
		fav_toggled_conn.block();
		fav_button->set_active(ncount>0);
		fav_toggled_conn.unblock();

		place_part_button->set_sensitive(part_current);
		fav_button->set_sensitive(part_current);
	}

	void PartBrowserWindow::handle_add_search() {
		auto ch = PoolBrowserPart::create(&pool);
		ch->get_style_context()->add_class("background");
		auto tab_label = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL, 0));
		auto la = Gtk::manage(new Gtk::Label("MPN Search"));
		auto close_button = Gtk::manage(new Gtk::Button());
		close_button->set_relief(Gtk::RELIEF_NONE);
		close_button->set_image_from_icon_name("window-close-symbolic");
		close_button->signal_clicked().connect([this, ch] {notebook->remove(*ch);});
		tab_label->pack_start(*close_button , false, false, 0);
		tab_label->pack_start(*la, true, true, 0);
		ch->show_all();
		tab_label->show_all();
		auto index = notebook->append_page(*ch, *tab_label);
		notebook->set_current_page(index);

		search_views.insert(ch);
		ch->unreference();
		ch->signal_part_selected().connect(sigc::mem_fun(this, &PartBrowserWindow::update_part_current));
		ch->signal_part_activated().connect(sigc::mem_fun(this, &PartBrowserWindow::handle_place_part));
	}

	PartBrowserWindow* PartBrowserWindow::create(Gtk::Window *p, const std::string &pool_path, std::deque<UUID> &favs) {
		PartBrowserWindow* w;
		Glib::RefPtr<Gtk::Builder> x = Gtk::Builder::create();
		x->add_from_resource("/net/carrotIndustries/horizon/prj-mgr/part_browser/part_browser.ui");
		x->get_widget_derived("window", w, pool_path, favs);

		return w;
	}
}
