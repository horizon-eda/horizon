#include "imp_symbol.hpp"
#include "part.hpp"
#include "symbol_preview/symbol_preview_window.hpp"

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
				{{GDK_KEY_d, GDK_KEY_y}, 	ToolID::DRAW_POLYGON},
				{{GDK_KEY_y}, 				ToolID::DRAW_POLYGON},
				{{GDK_KEY_d, GDK_KEY_c}, 	ToolID::DRAW_POLYGON_RECTANGLE},
		});
		key_seq.signal_update_hint().connect([this] (const std::string &s) {main_window->tool_hint_label->set_text(s);});

	}

	void ImpSymbol::canvas_update() {
		canvas->update(*core_symbol.get_canvas_data());
		symbol_preview_window->update(*core_symbol.get_canvas_data());
	}

	void ImpSymbol::construct() {

		symbol_preview_window = new SymbolPreviewWindow(main_window);
		symbol_preview_window->set_text_placements(core.y->get_symbol(false)->text_placements);

		{
			auto button = Gtk::manage(new Gtk::Button("Preview..."));
			main_window->top_panel->pack_start(*button, false, false, 0);
			button->show();
			button->signal_clicked().connect([this]{symbol_preview_window->present();});
		}


		name_entry = Gtk::manage(new Gtk::Entry());
		name_entry->show();
		main_window->top_panel->pack_start(*name_entry, false, false, 0);
		name_entry->set_text(core.y->get_symbol()->name);

		core.r->signal_save().connect([this]{
			auto sym = core.y->get_symbol(false);
			sym->name = name_entry->get_text();
			sym->text_placements = symbol_preview_window->get_text_placements();
		});
		grid_spin_button->set_sensitive(false);
	}

	ToolID ImpSymbol::handle_key(guint k) {
		return key_seq.handle_key(k);
	}
}
