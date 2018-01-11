#include "imp_schematic.hpp"
#include "export_pdf/export_pdf.hpp"
#include "pool/part.hpp"
#include "rules/rules_window.hpp"
#include "widgets/sheet_box.hpp"
#include "canvas/canvas_gl.hpp"

namespace horizon {
	ImpSchematic::ImpSchematic(const std::string &schematic_filename, const std::string &block_filename,  const PoolParams &pool_params) :ImpBase(pool_params),
			core_schematic(schematic_filename, block_filename, *pool)
	{
		core = &core_schematic;
		core_schematic.signal_tool_changed().connect(sigc::mem_fun(this, &ImpSchematic::handle_tool_change));
		core_schematic.signal_rebuilt().connect(sigc::mem_fun(this, &ImpSchematic::handle_core_rebuilt));

		key_seq_append_default(key_seq);
		key_seq.append_sequence({
				{{GDK_KEY_p, GDK_KEY_j}, 	ToolID::PLACE_JUNCTION},
				{{GDK_KEY_j},				ToolID::PLACE_JUNCTION},
				//{{GDK_KEY_d, GDK_KEY_l}, 	ToolID::DRAW_LINE},
				//{{GDK_KEY_l},				ToolID::DRAW_LINE},
				//{{GDK_KEY_d, GDK_KEY_a}, 	ToolID::DRAW_ARC},
				//{{GDK_KEY_a},				ToolID::DRAW_ARC},
				{{GDK_KEY_p, GDK_KEY_s},	ToolID::MAP_SYMBOL},
				{{GDK_KEY_s},				ToolID::MAP_SYMBOL},
				{{GDK_KEY_d, GDK_KEY_n}, 	ToolID::DRAW_NET},
				{{GDK_KEY_n},				ToolID::DRAW_NET},
				{{GDK_KEY_p, GDK_KEY_c},	ToolID::ADD_COMPONENT},
				{{GDK_KEY_c},				ToolID::ADD_COMPONENT},
				{{GDK_KEY_p, GDK_KEY_p},	ToolID::ADD_PART},
				{{GDK_KEY_P},				ToolID::ADD_PART},
				{{GDK_KEY_p, GDK_KEY_t},	ToolID::PLACE_TEXT},
				{{GDK_KEY_t},				ToolID::PLACE_TEXT},
				{{GDK_KEY_p, GDK_KEY_b},	ToolID::PLACE_NET_LABEL},
				{{GDK_KEY_b},				ToolID::PLACE_NET_LABEL},
				{{GDK_KEY_D},				ToolID::DISCONNECT},
				{{GDK_KEY_k},				ToolID::BEND_LINE_NET},
				{{GDK_KEY_g},				ToolID::SELECT_NET_SEGMENT},
				{{GDK_KEY_p, GDK_KEY_o},	ToolID::PLACE_POWER_SYMBOL},
				{{GDK_KEY_o},				ToolID::PLACE_POWER_SYMBOL},
				{{GDK_KEY_v},				ToolID::MOVE_NET_SEGMENT},
				{{GDK_KEY_V},				ToolID::MOVE_NET_SEGMENT_NEW},
				{{GDK_KEY_i},				ToolID::EDIT_SYMBOL_PIN_NAMES},
				{{GDK_KEY_p, GDK_KEY_u},	ToolID::PLACE_BUS_LABEL},
				{{GDK_KEY_u},				ToolID::PLACE_BUS_LABEL},
				{{GDK_KEY_p, GDK_KEY_r},	ToolID::PLACE_BUS_RIPPER},
				{{GDK_KEY_slash},			ToolID::PLACE_BUS_RIPPER},
				{{GDK_KEY_B},				ToolID::MANAGE_BUSES},
				{{GDK_KEY_h},				ToolID::SMASH},
				{{GDK_KEY_H},				ToolID::UNSMASH},
		});
		key_seq.signal_update_hint().connect([this] (const std::string &s) {main_window->tool_hint_label->set_text(s);});
	}

	void ImpSchematic::canvas_update() {
		canvas->update(*core_schematic.get_canvas_data());
		warnings_box->update(core_schematic.get_sheet()->warnings);
	}

	void ImpSchematic::handle_select_sheet(Sheet *sh) {
		if(sh == core_schematic.get_sheet())
			return;

		auto v = canvas->get_scale_and_offset();
		sheet_views[core_schematic.get_sheet()->uuid] = v;
		core_schematic.set_sheet(sh->uuid);
		canvas_update();
		if(sheet_views.count(sh->uuid)) {
			auto v2 = sheet_views.at(sh->uuid);
			canvas->set_scale_and_offset(v2.first, v2.second);
		}
		update_highlights();
	}

	void ImpSchematic::handle_remove_sheet(Sheet *sh) {
		core_schematic.delete_sheet(sh->uuid);
		canvas_update();
	}

	void ImpSchematic::update_highlights() {
		std::map<UUID, std::set<ObjectRef>> highlights_for_sheet;
		auto sch = core_schematic.get_schematic();
		for(const auto &it_sheet: sch->sheets) {
			auto sheet = &it_sheet.second;
			for(const auto &it: highlights) {
				if(it.type == ObjectType::NET) {
				for(const auto &it_line: sheet->net_lines) {
					if(it_line.second.net.uuid == it.uuid) {
							highlights_for_sheet[sheet->uuid].emplace(ObjectType::LINE_NET, it_line.first);
						}
					}
					for(const auto &it_junc: sheet->junctions) {
						if(it_junc.second.net.uuid == it.uuid) {
							highlights_for_sheet[sheet->uuid].emplace(ObjectType::JUNCTION, it_junc.first);
						}
					}
					for(const auto &it_label: sheet->net_labels) {
						if(it_label.second.junction->net.uuid == it.uuid) {
							highlights_for_sheet[sheet->uuid].emplace(ObjectType::NET_LABEL, it_label.first);
						}
					}
					for(const auto &it_sym: sheet->power_symbols) {
						if(it_sym.second.junction->net.uuid == it.uuid) {
							highlights_for_sheet[sheet->uuid].emplace(ObjectType::POWER_SYMBOL, it_sym.first);
						}
					}
					for(const auto &it_sym: sheet->symbols) {
						auto component = it_sym.second.component;
						for(const auto &it_pin: it_sym.second.symbol.pins) {
							UUIDPath<2> connpath(it_sym.second.gate->uuid, it_pin.second.uuid);
							if(component->connections.count(connpath)) {
								auto net = component->connections.at(connpath).net;
								if(net.uuid == it.uuid) {
									highlights_for_sheet[sheet->uuid].emplace(ObjectType::SYMBOL_PIN, it_pin.first, it_sym.first);
								}
							}
						}
					}
				}
				else if(it.type == ObjectType::COMPONENT) {
					for(const auto &it_sym: sheet->symbols) {
						auto component = it_sym.second.component;
						if(component->uuid == it.uuid) {
							highlights_for_sheet[sheet->uuid].emplace(ObjectType::SCHEMATIC_SYMBOL, it_sym.first);
						}
					}
				}
			}
			sheet_box->update_highlights(sheet->uuid, highlights_for_sheet[sheet->uuid].size());
		}

		auto sheet = core_schematic.get_sheet();
		canvas->set_flags_all(0, Triangle::FLAG_HIGHLIGHT);
		canvas->set_highlight_enabled(highlights_for_sheet[sheet->uuid].size());
		for(const auto &it: highlights_for_sheet[sheet->uuid]) {
			canvas->set_flags(it, Triangle::FLAG_HIGHLIGHT, 0);
		}
	}

	bool ImpSchematic::handle_broadcast(const json &j) {
		if(!ImpBase::handle_broadcast(j)) {
			std::string op = j.at("op");
			if(op == "place-part") {
				main_window->present();
				part_from_project_manager = j.at("part").get<std::string>();
				tool_begin(ToolID::ADD_PART);
			}
			else if(op == "highlight" && cross_probing_enabled) {
				if(!core_schematic.tool_is_active()) {
					highlights.clear();
					const json &o = j["objects"];
					for (auto it = o.cbegin(); it != o.cend(); ++it) {
						auto type = static_cast<ObjectType>(it.value().at("type").get<int>());
						UUID uu(it.value().at("uuid").get<std::string>());
						highlights.emplace(type, uu);
					}
					update_highlights();
				}
			}
		}
		return true;
	}

	void ImpSchematic::handle_selection_cross_probe() {
		json j;
		j["op"] = "schematic-select";
		j["selection"] = nullptr;
		for(const auto &it: canvas->get_selection()) {
			json k;
			ObjectType type = ObjectType::INVALID;
			UUID uu;
			auto sheet = core_schematic.get_sheet();
			switch(it.type) {
				case ObjectType::LINE_NET :{
					auto &li = sheet->net_lines.at(it.uuid);
					if(li.net) {
						type = ObjectType::NET;
						uu = li.net->uuid;
					}
				} break;
				case ObjectType::NET_LABEL :{
					auto &la = sheet->net_labels.at(it.uuid);
					if(la.junction->net) {
						type = ObjectType::NET;
						uu = la.junction->net->uuid;
					}
				} break;
				case ObjectType::POWER_SYMBOL :{
					auto &sym = sheet->power_symbols.at(it.uuid);
					if(sym.junction->net) {
						type = ObjectType::NET;
						uu = sym.junction->net->uuid;
					}
				} break;
				case ObjectType::JUNCTION :{
					auto &ju = sheet->junctions.at(it.uuid);
					if(ju.net) {
						type = ObjectType::NET;
						uu = ju.net->uuid;
					}
				} break;
				case ObjectType::SCHEMATIC_SYMBOL :{
					auto &sym = sheet->symbols.at(it.uuid);
					type = ObjectType::COMPONENT;
					uu = sym.component->uuid;
				} break;
				default:;
			}

			if(type != ObjectType::INVALID) {
				k["type"] = static_cast<int>(type);
				k["uuid"] = (std::string)uu;
				j["selection"].push_back(k);
			}
		}
		send_json(j);
	}

	void ImpSchematic::construct() {

		main_window->set_title("Schematic - Interactive Manipulator");
		state_store = std::make_unique<WindowStateStore>(main_window, "imp-schematic");

		sheet_box = Gtk::manage(new SheetBox(&core_schematic));
		sheet_box->show_all();
		sheet_box->signal_add_sheet().connect([this]{core_schematic.add_sheet(); std::cout<<"add sheet"<<std::endl;});
		sheet_box->signal_remove_sheet().connect(sigc::mem_fun(this, &ImpSchematic::handle_remove_sheet));
		sheet_box->signal_select_sheet().connect(sigc::mem_fun(this, &ImpSchematic::handle_select_sheet));
		main_window->left_panel->pack_start(*sheet_box, false, false, 0);

		auto hamburger_menu = add_hamburger_menu();
		hamburger_menu->append("Annotate", "win.annotate");
		add_tool_action(ToolID::ANNOTATE, "annotate");

		hamburger_menu->append("Buses...", "win.manage_buses");
		add_tool_action(ToolID::MANAGE_BUSES, "manage_buses");

		hamburger_menu->append("Net classes...", "win.manage_nc");
		add_tool_action(ToolID::MANAGE_NET_CLASSES, "manage_nc");

		hamburger_menu->append("Power Nets...", "win.manage_pn");
		add_tool_action(ToolID::MANAGE_POWER_NETS, "manage_pn");

		hamburger_menu->append("Schematic properties", "win.sch_properties");
		add_tool_action(ToolID::EDIT_SCHEMATIC_PROPERTIES, "sch_properties");

		hamburger_menu->append("Export PDF", "win.export_pdf");

		main_window->add_action("export_pdf", [this] {
			handle_export_pdf();
		});

		if(sockets_connected) {
			canvas->signal_selection_changed().connect(sigc::mem_fun(this, &ImpSchematic::handle_selection_cross_probe));
			hamburger_menu->append("Cross probing", "win.cross_probing");
			auto cp_action = main_window->add_action_bool("cross_probing", true);
			cross_probing_enabled = true;
			cp_action->signal_change_state().connect([this, cp_action] (const Glib::VariantBase& v) {
				cross_probing_enabled = Glib::VariantBase::cast_dynamic<Glib::Variant<bool>>(v).get();
				g_simple_action_set_state(cp_action->gobj(), g_variant_new_boolean(cross_probing_enabled));
				if(!cross_probing_enabled && !core_schematic.tool_is_active()) {
					highlights.clear();
					update_highlights();
				}
			});
		}

		canvas->set_highlight_mode(CanvasGL::HighlightMode::DIM);

		if(!sockets_connected) {
			add_tool_button(ToolID::ADD_PART, "Place part", false);
		}
		else {
			auto button = Gtk::manage(new Gtk::Button("Place part"));
			button->signal_clicked().connect([this]{
				json j;
				j["op"] = "show-browser";
				send_json(j);
			});
			button->show();
			core.r->signal_tool_changed().connect([button](ToolID t){button->set_sensitive(t==ToolID::NONE);});
			main_window->header->pack_end(*button);
		}

		if(sockets_connected){
			auto button = Gtk::manage(new Gtk::Button("To board"));
			button->set_tooltip_text("Place selected components on board");
			button->signal_clicked().connect([this]{
				json j;
				j["op"] = "to-board";
				j["selection"] = nullptr;
				for(const auto &it: canvas->get_selection()) {
					auto sheet = core_schematic.get_sheet();
					if(it.type == ObjectType::SCHEMATIC_SYMBOL) {
						auto &sym = sheet->symbols.at(it.uuid);
						json k;
						k["type"] = static_cast<int>(ObjectType::COMPONENT);
						k["uuid"] = (std::string)sym.component->uuid;
						j["selection"].push_back(k);
					}
				}
				send_json(j);
			});
			button->show();
			core.r->signal_tool_changed().connect([button](ToolID t){button->set_sensitive(t==ToolID::NONE);});
			main_window->header->pack_end(*button);
		}

		grid_spin_button->set_sensitive(false);

		rules_window->signal_goto().connect([this] (Coordi location, UUID sheet) {
			auto sch = core_schematic.get_schematic();
			if(sch->sheets.count(sheet)) {
				sheet_box->select_sheet(sheet);
				canvas->center_and_zoom(location);
			}
		});

		canvas->signal_motion_notify_event().connect([this] (GdkEventMotion *ev){
			if(target_drag_begin.type != ObjectType::INVALID) {
				handle_drag();
			}
			return false;
		});

		canvas->signal_button_release_event().connect([this] (GdkEventButton *ev){
			target_drag_begin = Target();
			return false;
		});

	}

	void ImpSchematic::handle_export_pdf() {
		GtkFileChooserNative *native = gtk_file_chooser_native_new ("Save PDF",
			GTK_WINDOW(main_window->gobj()),
			GTK_FILE_CHOOSER_ACTION_SAVE,
			"_Open",
			"_Cancel");
		auto chooser = Glib::wrap(GTK_FILE_CHOOSER(native));
		chooser->set_do_overwrite_confirmation(true);
		if(last_pdf_filename.size()) {
			chooser->set_filename(last_pdf_filename);
		}
		else {
			chooser->set_current_name("schematic.pdf");
		}

		if(gtk_native_dialog_run (GTK_NATIVE_DIALOG (native))==GTK_RESPONSE_ACCEPT) {
			std::string fn = chooser->get_filename();
			last_pdf_filename = fn;
			export_pdf(fn, *core.c->get_schematic(), core.r);
		}
	}

	void ImpSchematic::handle_core_rebuilt() {
		sheet_box->update();
	}

	void ImpSchematic::handle_tool_change(ToolID id) {
		ImpBase::handle_tool_change(id);
		sheet_box->set_sensitive(id == ToolID::NONE);
	}

	ToolID ImpSchematic::handle_key(guint k) {
		return key_seq.handle_key(k);
	}

	void ImpSchematic::handle_maybe_drag() {
		if(!preferences.schematic.drag_start_net_line)
			return;
		auto target = canvas->get_current_target();
		if(target.type == ObjectType::SYMBOL_PIN || target.type == ObjectType::JUNCTION) {
			std::cout << "click pin" << std::endl;
			canvas->inhibit_drag_selection();
			target_drag_begin = target;
			cursor_pos_drag_begin = canvas->get_cursor_pos_win();
		}
	}

	static bool drag_does_start(const Coordf &delta, Orientation orientation) {
		float thr = 50;
		switch(orientation) {
			case Orientation::DOWN :
				return delta.y > thr;

			case Orientation::UP :
				return -delta.y > thr;

			case Orientation::RIGHT :
				return delta.x > thr;

			case Orientation::LEFT :
				return -delta.x > thr;

			default :
				return false;
		}
	}

	void ImpSchematic::handle_drag() {
		auto pos = canvas->get_cursor_pos_win();
		auto delta = pos-cursor_pos_drag_begin;
		bool start = false;

		if(target_drag_begin.type == ObjectType::SYMBOL_PIN) {
			const auto sym = core_schematic.get_schematic_symbol(target_drag_begin.path.at(0));
			const auto &pin = sym->symbol.pins.at(target_drag_begin.path.at(1));
			auto orientation = pin.get_orientation_for_placement(sym->placement);
			start = drag_does_start(delta, orientation);
		}
		else if(target_drag_begin.type == ObjectType::JUNCTION) {
			start = delta.mag_sq()>(50*50);
		}

		if(start) {
			{
				ToolArgs args;
				args.coords = target_drag_begin.p;
				ToolResponse r= core.r->tool_begin(ToolID::DRAW_NET, args, imp_interface.get());
				tool_process(r);
			}
			{
				ToolArgs args;
				args.type = ToolEventType::CLICK;
				args.coords = target_drag_begin.p;
				args.button = 1;
				args.target = target_drag_begin;
				args.work_layer = canvas->property_work_layer();
				ToolResponse r = core.r->tool_update(args);
				tool_process(r);
			}
			target_drag_begin = Target();
		}
	}
}
