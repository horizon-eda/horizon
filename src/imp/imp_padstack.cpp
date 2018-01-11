#include "imp_padstack.hpp"
#include "pool/part.hpp"
#include "parameter_window.hpp"
#include "header_button.hpp"
#include "canvas/canvas_gl.hpp"

namespace horizon {
	ImpPadstack::ImpPadstack(const std::string &padstack_filename, const std::string &pool_path):
			ImpLayer(pool_path),
			core_padstack(padstack_filename, *pool) {
		core = &core_padstack;
		core_padstack.signal_tool_changed().connect(sigc::mem_fun(this, &ImpBase::handle_tool_change));
		key_seq_append_default(key_seq);
		key_seq.append_sequence({
				{{GDK_KEY_d, GDK_KEY_y}, 	ToolID::DRAW_POLYGON},
				{{GDK_KEY_p, GDK_KEY_s}, 	ToolID::PLACE_SHAPE},
				{{GDK_KEY_p, GDK_KEY_h}, 	ToolID::PLACE_HOLE},
				{{GDK_KEY_y}, 				ToolID::DRAW_POLYGON},
				{{GDK_KEY_d, GDK_KEY_r}, 	ToolID::DRAW_POLYGON_RECTANGLE},
				{{GDK_KEY_h}, 				ToolID::PLACE_HOLE},
				{{GDK_KEY_s}, 				ToolID::PLACE_SHAPE},
				{{GDK_KEY_i}, 				ToolID::EDIT_SHAPE}
		});
		key_seq.signal_update_hint().connect([this] (const std::string &s) {main_window->tool_hint_label->set_text(s);});
		//core_symbol.signal_rebuilt().connect(sigc::mem_fun(this, &ImpBase::handle_core_rebuilt));
	}

	void ImpPadstack::canvas_update() {
		canvas->update(*core_padstack.get_canvas_data());

	}

	void ImpPadstack::construct() {
		ImpLayer::construct_layer_box();

		main_window->set_title("Padstack - Interactive Manipulator");
		state_store = std::make_unique<WindowStateStore>(main_window, "imp-padstack");

		auto header_button = Gtk::manage(new HeaderButton);
		header_button->set_label(core_padstack.get_padstack(false)->name);
		main_window->header->set_custom_title(*header_button);
		header_button->show();

		auto name_entry = header_button->add_entry("Name");
		name_entry->set_text(core_padstack.get_padstack(false)->name);
		name_entry->set_width_chars(core_padstack.get_padstack(false)->name.size());
		name_entry->signal_changed().connect([name_entry, header_button] {header_button->set_label(name_entry->get_text());});

		core_padstack.signal_save().connect([this, name_entry, header_button]{
			core_padstack.get_padstack(false)->name = name_entry->get_text();
			header_button->set_label(core_padstack.get_padstack(false)->name);
		});


		auto type_combo = Gtk::manage(new Gtk::ComboBoxText());
		type_combo->append("top", "Top");
		type_combo->append("bottom", "Bottom");
		type_combo->append("through", "Through");
		type_combo->append("via", "Via");
		type_combo->append("hole", "Hole");
		type_combo->append("mechanical", "Mechanical");
		type_combo->show();
		header_button->add_widget("Type", type_combo);
		type_combo->set_active_id(Padstack::type_lut.lookup_reverse(core_padstack.get_padstack(false)->type));

		auto parameter_window = new ParameterWindow(main_window, &core_padstack.parameter_program_code, &core_padstack.parameter_set);
		{
			auto button = Gtk::manage(new Gtk::Button("Parameters..."));
			main_window->header->pack_start(*button);
			button->show();
			button->signal_clicked().connect([this, parameter_window]{parameter_window->present();});
		}
		parameter_window->signal_apply().connect([this, parameter_window] {
			if(core.r->tool_is_active())
				return;
			auto ps = core_padstack.get_padstack(false);
			auto r_compile = ps->parameter_program.set_code(core_padstack.parameter_program_code);
			if(r_compile.first) {
				parameter_window->set_error_message("<b>Compile error:</b>"+r_compile.second);
				return;
			}
			else {
				parameter_window->set_error_message("");
			}
			ps->parameter_set = core_padstack.parameter_set;
			auto r = ps->parameter_program.run(ps->parameter_set);
			if(r.first) {
				parameter_window->set_error_message("<b>Run error:</b>"+r.second);
				return;
			}
			else {
				parameter_window->set_error_message("");
			}
			core_padstack.rebuild();
			canvas_update();
		});
		core.r->signal_tool_changed().connect([parameter_window](ToolID t){parameter_window->set_can_apply(t==ToolID::NONE);});

		core_padstack.signal_save().connect([this, type_combo]{core_padstack.get_padstack(false)->type = Padstack::type_lut.lookup(type_combo->get_active_id());});

	}

	ToolID ImpPadstack::handle_key(guint k) {
		return key_seq.handle_key(k);
	}
};
