#include "imp_symbol.hpp"
#include "part.hpp"

namespace horizon {
	ImpSymbol::ImpSymbol(const std::string &symbol_filename, const std::string &pool_path):
			ImpBase(pool_path),
			core_symbol(symbol_filename, pool) {
		core = &core_symbol;
		core_symbol.signal_tool_changed().connect(sigc::mem_fun(this, &ImpBase::handle_tool_change));

		key_seq_append_default(key_seq);
		key_seq.append_sequence({
				{{GDK_KEY_p, GDK_KEY_j}, 	ToolID::PLACE_JUNCTION},
				{{GDK_KEY_j},				ToolID::PLACE_JUNCTION},
				{{GDK_KEY_d, GDK_KEY_l}, 	ToolID::DRAW_LINE},
				{{GDK_KEY_l},				ToolID::DRAW_LINE},
				{{GDK_KEY_d, GDK_KEY_a}, 	ToolID::DRAW_ARC},
				{{GDK_KEY_a},				ToolID::DRAW_ARC},
				{{GDK_KEY_d, GDK_KEY_r},	ToolID::DRAW_LINE_RECTANGLE},
				{{GDK_KEY_i},				ToolID::EDIT_LINE_RECTANGLE},
				{{GDK_KEY_p, GDK_KEY_p},	ToolID::MAP_PIN},
				{{GDK_KEY_p, GDK_KEY_t},	ToolID::PLACE_TEXT},
				{{GDK_KEY_t},				ToolID::PLACE_TEXT},
		});
		key_seq.signal_update_hint().connect([this] (const std::string &s) {main_window->tool_hint_label->set_text(s);});

	}

	void ImpSymbol::canvas_update() {
		canvas->update(*core_symbol.get_canvas_data());
	}

	void ImpSymbol::construct() {

		name_entry = Gtk::manage(new Gtk::Entry());
		name_entry->show();
		main_window->top_panel->pack_start(*name_entry, false, false, 0);
		name_entry->set_text(core.y->get_symbol()->name);

		core.r->signal_save().connect([this]{core.y->get_symbol(false)->name = name_entry->get_text();});
		grid_spin_button->set_sensitive(false);
	}

	ToolID ImpSymbol::handle_key(guint k) {
		return key_seq.handle_key(k);
	}
}
