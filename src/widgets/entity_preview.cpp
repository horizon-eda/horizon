#include "entity_preview.hpp"
#include "pool/entity.hpp"
#include "pool/pool.hpp"
#include "canvas/canvas.hpp"
#include "util/util.hpp"
#include "util/sqlite.hpp"
#include "common/object_descr.hpp"
#include "preview_canvas.hpp"

namespace horizon {
	EntityPreview::EntityPreview(class Pool &p, bool show_goto): Gtk::Box(Gtk::ORIENTATION_VERTICAL, 0), pool(p) {

		auto symbol_sel_box = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL, 4));
		symbol_sel_box->property_margin() = 8;
		{
			auto la = Gtk::manage(new Gtk::Label("Gate"));
			la->get_style_context()->add_class("dim-label");
			symbol_sel_box->pack_start(*la, false, false, 0);
		}
		combo_gate = Gtk::manage(new Gtk::ComboBoxText);
		combo_gate->signal_changed().connect(sigc::mem_fun(this, &EntityPreview::handle_gate_sel));
		symbol_sel_box->pack_start(*combo_gate, true, true, 0);

		if(show_goto) {
			auto bu = create_goto_button(ObjectType::UNIT, [this] {
				return entity->gates.at(UUID(combo_gate->get_active_id())).unit->uuid;
			});
			symbol_sel_box->pack_start(*bu, false, false, 0);
		}

		{
			auto la = Gtk::manage(new Gtk::Label("Symbol"));
			la->get_style_context()->add_class("dim-label");
			symbol_sel_box->pack_start(*la, false, false, 0);
			la->set_margin_start(4);
		}
		combo_symbol = Gtk::manage(new Gtk::ComboBoxText);
		combo_symbol->signal_changed().connect(sigc::mem_fun(this, &EntityPreview::handle_symbol_sel));
		symbol_sel_box->pack_start(*combo_symbol, true, true, 0);

		if(show_goto) {
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

	void EntityPreview::load(const Entity *e) {
		entity = e;
		combo_gate->remove_all();

		for(auto it: goto_buttons) {
			it->set_sensitive(entity);
		}
		if(!entity) {
			canvas_symbol->clear();
			return;
		}

		std::vector<const Gate*> gates;
		gates.reserve(entity->gates.size());
		for(const auto &it: entity->gates) {
			gates.push_back(&it.second);
		}
		std::sort(gates.begin(), gates.end(), [](const auto *a, const auto *b){return strcmp_natural(a->suffix, b->suffix)<0;});

		for(const auto it: gates) {
			if(it->suffix.size())
				combo_gate->append((std::string)it->uuid, it->suffix + ": " + it->unit->name);
			else
				combo_gate->append((std::string)it->uuid, it->unit->name);
		}
		combo_gate->set_active(0);

	}

	void EntityPreview::handle_gate_sel() {
		combo_symbol->remove_all();
		if(!entity)
			return;
		if(combo_gate->get_active_row_number() == -1)
			return;

		const Gate *gate = &entity->gates.at(UUID(combo_gate->get_active_id()));
		auto unit = gate->unit;

		SQLite::Query q(pool.db, "SELECT uuid, name FROM symbols WHERE unit=? ORDER BY name");
		q.bind(1, unit->uuid);
		while(q.step()) {
			UUID uu = q.get<std::string>(0);
			std::string name = q.get<std::string>(1);
			combo_symbol->append((std::string)uu, name);
		}
		combo_symbol->set_active(0);
	}

	void EntityPreview::handle_symbol_sel() {
		canvas_symbol->clear();
		if(!entity)
			return;
		if(combo_symbol->get_active_row_number() == -1)
			return;

		canvas_symbol->load(ObjectType::SYMBOL, UUID(combo_symbol->get_active_id()));
	}

}
