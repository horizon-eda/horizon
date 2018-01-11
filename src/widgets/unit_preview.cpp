#include "unit_preview.hpp"
#include "pool/unit.hpp"
#include "pool/pool.hpp"
#include "canvas/canvas.hpp"
#include "util/util.hpp"
#include "util/sqlite.hpp"
#include "common/object_descr.hpp"
#include "preview_canvas.hpp"

namespace horizon {
	UnitPreview::UnitPreview(class Pool &p): Gtk::Box(Gtk::ORIENTATION_VERTICAL, 0), pool(p) {

		auto symbol_sel_box = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL, 4));
		symbol_sel_box->property_margin() = 8;
		{
			auto la = Gtk::manage(new Gtk::Label("Symbol"));
			la->get_style_context()->add_class("dim-label");
			symbol_sel_box->pack_start(*la, false, false, 0);
		}
		combo_symbol = Gtk::manage(new Gtk::ComboBoxText);
		combo_symbol->signal_changed().connect(sigc::mem_fun(this, &UnitPreview::handle_symbol_sel));
		symbol_sel_box->pack_start(*combo_symbol, true, true, 0);

		{
			auto bu = create_goto_button(ObjectType::SYMBOL, [this] {
				return UUID(combo_symbol->get_active_id());
			});
			symbol_sel_box->pack_start(*bu, false, false, 0);
		}

		pack_start(*symbol_sel_box, false, false, 0);

		{
			auto sep = Gtk::manage(new Gtk::Separator(Gtk::ORIENTATION_HORIZONTAL));
			sep->show();
			pack_start(*sep, false, false, 0);
		}

		canvas_symbol = Gtk::manage(new PreviewCanvas(pool));
		canvas_symbol->show();
		pack_start(*canvas_symbol, true, true, 0);


		load(nullptr);
	}

	void UnitPreview::load(const Unit *u) {
		unit = u;

		for(auto it: goto_buttons) {
			it->set_sensitive(unit);
		}
		combo_symbol->remove_all();
		if(!unit) {
			return;
		}

		SQLite::Query q(pool.db, "SELECT uuid, name FROM symbols WHERE unit=? ORDER BY name");
		q.bind(1, unit->uuid);
		while(q.step()) {
			UUID uu = q.get<std::string>(0);
			std::string name = q.get<std::string>(1);
			combo_symbol->append((std::string)uu, name);
		}
		combo_symbol->set_active(0);
	}

	void UnitPreview::handle_symbol_sel() {
		canvas_symbol->clear();
		if(!unit)
			return;
		if(combo_symbol->get_active_row_number() == -1)
			return;

		canvas_symbol->load(ObjectType::SYMBOL, UUID(combo_symbol->get_active_id()));
	}

}
