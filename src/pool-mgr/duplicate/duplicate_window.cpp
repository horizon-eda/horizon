#include "duplicate_window.hpp"
#include "duplicate_unit.hpp"
#include "duplicate_entity.hpp"
#include "duplicate_part.hpp"
#include "common/object_descr.hpp"

namespace horizon {
	DuplicateWindow::DuplicateWindow(class Pool *p, ObjectType ty, const UUID &uu): Gtk::Window(), pool(p) {
		set_type_hint(Gdk::WINDOW_TYPE_HINT_DIALOG);
		auto hb = Gtk::manage(new Gtk::HeaderBar());
		set_titlebar(*hb);

		auto duplicate_button = Gtk::manage(new Gtk::Button("Duplicate"));
		hb->pack_start(*duplicate_button);

		hb->show_all();
		hb->set_show_close_button(true);

		auto box = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL, 20));
		box->property_margin() = 20;
		box->show();
		add(*box);

		set_title("Duplicate "+object_descriptions.at(ty).name);

		if(ty == ObjectType::UNIT) {
			auto w = Gtk::manage(new DuplicateUnitWidget(pool, uu, false, this));
			box->pack_start(*w, true, true, 0);
			w->show();
			duplicate_button->signal_clicked().connect([w] {w->duplicate();});
		}
		else if(ty == ObjectType::ENTITY) {
			auto ubox = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_VERTICAL, 10));
			auto w = Gtk::manage(new DuplicateEntityWidget(pool, uu, ubox, false, this));
			box->pack_start(*w, true, true, 0);
			box->pack_start(*ubox, true, true, 0);
			w->show();
			ubox->show();
			duplicate_button->signal_clicked().connect([w] {w->duplicate();});
		}
		else if(ty == ObjectType::PART) {
			auto ubox = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_VERTICAL, 10));
			auto w = Gtk::manage(new DuplicatePartWidget(pool, uu, ubox, this));
			box->pack_start(*w, true, true, 0);
			box->pack_start(*ubox, true, true, 0);
			w->show();
			ubox->show();
			duplicate_button->signal_clicked().connect([w] {w->duplicate();});
		}
	}

	bool DuplicateWindow::get_duplicated() const {
		return duplicated;
	}
}
