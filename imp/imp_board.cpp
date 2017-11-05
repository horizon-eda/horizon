#include "imp_board.hpp"
#include "part.hpp"
#include "rules/rules_window.hpp"
#include "canvas/canvas_patch.hpp"
#include "fab_output_window.hpp"
#include "3d_view.hpp"

namespace horizon {
	ImpBoard::ImpBoard(const std::string &board_filename, const std::string &block_filename, const std::string &via_dir, const std::string &pool_path):
			ImpLayer(pool_path),
			core_board(board_filename, block_filename, via_dir, pool) {
		core = &core_board;
		core_board.signal_tool_changed().connect(sigc::mem_fun(this, &ImpBase::handle_tool_change));


		key_seq_append_default(key_seq);
		key_seq.append_sequence({
				{{GDK_KEY_p, GDK_KEY_j}, 	ToolID::PLACE_JUNCTION},
				{{GDK_KEY_j},				ToolID::PLACE_JUNCTION},
				{{GDK_KEY_d, GDK_KEY_l}, 	ToolID::DRAW_LINE},
				{{GDK_KEY_l},				ToolID::DRAW_LINE},
				{{GDK_KEY_d, GDK_KEY_a}, 	ToolID::DRAW_ARC},
				{{GDK_KEY_a},				ToolID::DRAW_ARC},
				{{GDK_KEY_d, GDK_KEY_y}, 	ToolID::DRAW_POLYGON},
				{{GDK_KEY_y}, 				ToolID::DRAW_POLYGON},
				{{GDK_KEY_d, GDK_KEY_r}, 	ToolID::DRAW_POLYGON_RECTANGLE},
				{{GDK_KEY_p, GDK_KEY_t},	ToolID::PLACE_TEXT},
				{{GDK_KEY_t},				ToolID::PLACE_TEXT},
				{{GDK_KEY_p, GDK_KEY_p},	ToolID::MAP_PACKAGE},
				{{GDK_KEY_P},				ToolID::MAP_PACKAGE},
				{{GDK_KEY_d, GDK_KEY_t},	ToolID::DRAW_TRACK},
				{{GDK_KEY_p, GDK_KEY_v},	ToolID::PLACE_VIA},
				{{GDK_KEY_v},				ToolID::PLACE_VIA},
				{{GDK_KEY_d, GDK_KEY_d}, 	ToolID::DRAW_DIMENSION},
				{{GDK_KEY_X},				ToolID::ROUTE_TRACK},
				{{GDK_KEY_x},				ToolID::ROUTE_TRACK_INTERACTIVE},
				{{GDK_KEY_g},				ToolID::DRAG_KEEP_SLOPE},
				{{GDK_KEY_q},				ToolID::UPDATE_ALL_PLANES},
				{{GDK_KEY_Q},				ToolID::CLEAR_ALL_PLANES},
		});
		key_seq.signal_update_hint().connect([this] (const std::string &s) {main_window->tool_hint_label->set_text(s);});

	}

	void ImpBoard::canvas_update() {
		canvas->update(*core_board.get_canvas_data());
		warnings_box->update(core_board.get_board()->warnings);
		update_highlights();
	}

	void ImpBoard::update_highlights() {
		canvas->set_flags_all(0, Triangle::FLAG_HIGHLIGHT);
		canvas->set_highlight_enabled(highlights.size());
		for(const auto &it: highlights) {
			if(it.type == ObjectType::NET) {
				for(const auto &it_track: core_board.get_board()->tracks) {
					if(it_track.second.net.uuid == it.uuid) {
						ObjectRef ref(ObjectType::TRACK, it_track.first);
						canvas->set_flags(ref, Triangle::FLAG_HIGHLIGHT, 0);
					}
				}
				for(const auto &it_via: core_board.get_board()->vias) {
					if(it_via.second.junction->net.uuid == it.uuid) {
						ObjectRef ref(ObjectType::VIA, it_via.first);
						canvas->set_flags(ref, Triangle::FLAG_HIGHLIGHT, 0);
					}
				}
				for(const auto &it_pkg: core_board.get_board()->packages) {
					for(const auto &it_pad: it_pkg.second.package.pads) {
						if(it_pad.second.net.uuid == it.uuid) {
							ObjectRef ref(ObjectType::PAD, it_pad.first, it_pkg.first);
							canvas->set_flags(ref, Triangle::FLAG_HIGHLIGHT, 0);
						}
					}
				}
			}
			if(it.type == ObjectType::COMPONENT) {
				for(const auto &it_pkg: core_board.get_board()->packages) {
					if(it_pkg.second.component->uuid == it.uuid) {
						ObjectRef ref(ObjectType::BOARD_PACKAGE, it_pkg.first);
						canvas->set_flags(ref, Triangle::FLAG_HIGHLIGHT, 0);
					}
				}
			}
			else {
				canvas->set_flags(it, Triangle::FLAG_HIGHLIGHT, 0);
			}
		}
	}

	bool ImpBoard::handle_broadcast(const json &j) {
		if(!ImpBase::handle_broadcast(j)) {
			std::string op = j.at("op");
			if(op == "highlight" && cross_probing_enabled) {
				if(!core_board.tool_is_active()) {
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


	void ImpBoard::construct() {
		ImpLayer::construct();

		auto view_3d_button = Gtk::manage(new Gtk::Button("3D"));
		main_window->header->pack_start(*view_3d_button);
		view_3d_button->show();
		view_3d_button->signal_clicked().connect([this]{
			view_3d_window->update(); view_3d_window->present();
		});


		auto hamburger_menu = add_hamburger_menu();

		hamburger_menu->append("Fabrication output", "win.fab_out");
		main_window->add_action("fab_out", [this] {
			fab_output_window->present();
		});

		hamburger_menu->append("Stackup...", "win.edit_stackup");
		add_tool_action(ToolID::EDIT_STACKUP, "edit_stackup");

		hamburger_menu->append("Reload netlist", "win.reload_netlist");
		main_window->add_action("reload_netlist", [this] {
			core_board.reload_netlist();
			canvas_update();
		});

		hamburger_menu->append("Update all planes", "win.update_all_planes");
		add_tool_action(ToolID::UPDATE_ALL_PLANES, "update_all_planes");

		hamburger_menu->append("Clear all planes", "win.clear_all_planes");
		add_tool_action(ToolID::CLEAR_ALL_PLANES, "clear_all_planes");

		if(sockets_connected) {
			hamburger_menu->append("Cross probing", "win.cross_probing");
			auto cp_action = main_window->add_action_bool("cross_probing", true);
			cross_probing_enabled = true;
			cp_action->signal_change_state().connect([this, cp_action] (const Glib::VariantBase& v) {
				cross_probing_enabled = Glib::VariantBase::cast_dynamic<Glib::Variant<bool>>(v).get();
				cp_action->set_state(v);
				if(!cross_probing_enabled && !core_board.tool_is_active()) {
					highlights.clear();
					update_highlights();
				}
			});
		}



		add_tool_button(ToolID::MAP_PACKAGE, "Place package", false);

		/*auto test_button = Gtk::manage(new Gtk::Button("Test"));
		main_window->top_panel->pack_start(*test_button, false, false, 0);
		test_button->show();
		test_button->signal_clicked().connect([this] {
			std::ofstream ofs("/tmp/patches");
			int i = 0;
			for(const auto &it: cp.patches) {
				if(it.first.layer != 10000)
					continue;
				for(const auto &itp: it.second) {
					ofs << "#" << static_cast<int>(it.first.type) << " " << it.first.layer << " " << (std::string)it.first.net << "\n";
					for(const auto &itc: itp) {
						ofs << itc.X << " " << itc.Y << " " << i << "\n";
					}
					ofs << itp.front().X << " " << itp.front().Y << " " << i  << "\n\n";
				}
				ofs << "\n";
				i++;
			}


		});*/


		fab_output_window = FabOutputWindow::create(main_window, core.b->get_board(), core.b->get_fab_output_settings());
		view_3d_window = View3DWindow::create(core_board.get_board());

		core.r->signal_tool_changed().connect([this](ToolID t){
			fab_output_window->set_can_generate(t==ToolID::NONE);
		});

		rules_window->signal_goto().connect([this] (Coordi location, UUID sheet) {
			canvas->center_and_zoom(location);
		});
	}

	ToolID ImpBoard::handle_key(guint k) {
		return key_seq.handle_key(k);
	}
}
