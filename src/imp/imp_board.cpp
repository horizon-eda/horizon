#include "imp_board.hpp"
#include "pool/part.hpp"
#include "rules/rules_window.hpp"
#include "canvas/canvas_pads.hpp"
#include "fab_output_window.hpp"
#include "widgets/layer_box.hpp"
#include "widgets/board_display_options.hpp"
#include "3d_view.hpp"
#include "canvas/canvas_gl.hpp"

namespace horizon {
	ImpBoard::ImpBoard(const std::string &board_filename, const std::string &block_filename, const std::string &via_dir,  const PoolParams &pool_params):
			ImpLayer(pool_params),
			core_board(board_filename, block_filename, via_dir, *pool) {
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
				{{GDK_KEY_X},				ToolID::ROUTE_DIFFPAIR_INTERACTIVE},
				{{GDK_KEY_x},				ToolID::ROUTE_TRACK_INTERACTIVE},
				{{GDK_KEY_G},				ToolID::DRAG_KEEP_SLOPE},
				{{GDK_KEY_g},				ToolID::DRAG_TRACK_INTERACTIVE},
				{{GDK_KEY_q},				ToolID::UPDATE_ALL_PLANES},
				{{GDK_KEY_Q},				ToolID::CLEAR_ALL_PLANES},
				{{GDK_KEY_o},				ToolID::SELECT_MORE},
				{{GDK_KEY_O},				ToolID::SELECT_NET_SEGMENT},
				{{GDK_KEY_c},				ToolID::LOCK},
				{{GDK_KEY_C},				ToolID::UNLOCK_ALL},
				{{GDK_KEY_h},				ToolID::PLACE_BOARD_HOLE},
				{{GDK_KEY_p, GDK_KEY_h},	ToolID::PLACE_BOARD_HOLE},
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
			if(op == "highlight" && cross_probing_enabled && !core_board.tool_is_active()) {
				highlights.clear();
				const json &o = j["objects"];
				for (auto it = o.cbegin(); it != o.cend(); ++it) {
					auto type = static_cast<ObjectType>(it.value().at("type").get<int>());
					UUID uu(it.value().at("uuid").get<std::string>());
					highlights.emplace(type, uu);
				}
				update_highlights();
			}
			else if(op == "place" && !core_board.tool_is_active()) {
				main_window->present();
				std::set<SelectableRef> components;
				const json &o = j["components"];
				for (auto it = o.cbegin(); it != o.cend(); ++it) {
					auto type = static_cast<ObjectType>(it.value().at("type").get<int>());
					if(type == ObjectType::COMPONENT) {
						UUID uu(it.value().at("uuid").get<std::string>());
						components.emplace(uu, type);
					}
				}
				ToolArgs args;
				args.coords = canvas->get_cursor_pos();
				args.selection = components;
				args.work_layer = canvas->property_work_layer();
				ToolResponse r= core.r->tool_begin(ToolID::MAP_PACKAGE, args, imp_interface.get());
				tool_process(r);
			}
		}
		return true;
	}

	void ImpBoard::handle_selection_cross_probe() {
		json j;
		j["op"] = "board-select";
		j["selection"] = nullptr;
		for(const auto &it: canvas->get_selection()) {
			json k;
			ObjectType type = ObjectType::INVALID;
			UUID uu;
			auto board = core_board.get_board();
			switch(it.type) {
				case ObjectType::TRACK :{
					auto &track = board->tracks.at(it.uuid);
					if(track.net) {
						type = ObjectType::NET;
						uu = track.net->uuid;
					}
				} break;
				case ObjectType::VIA :{
					auto &via= board->vias.at(it.uuid);
					if(via.junction->net) {
						type = ObjectType::NET;
						uu = via.junction->net->uuid;
					}
				} break;
				case ObjectType::JUNCTION :{
					auto &ju = board->junctions.at(it.uuid);
					if(ju.net) {
						type = ObjectType::NET;
						uu = ju.net->uuid;
					}
				} break;
				case ObjectType::BOARD_PACKAGE :{
					auto &pkg = board->packages.at(it.uuid);
					type = ObjectType::COMPONENT;
					uu = pkg.component->uuid;
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

	void transform_path(ClipperLib::Path &path, const Placement &tr) {
		for(auto &it: path) {
			Coordi p(it.X, it.Y);
			auto q = tr.transform(p);
			it.X = q.x;
			it.Y = q.y;
		}
	}


	void ImpBoard::construct() {
		ImpLayer::construct_layer_box(false);

		main_window->set_title("Board - Interactive Manipulator");
		state_store = std::make_unique<WindowStateStore>(main_window, "imp-board");

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
				g_simple_action_set_state(cp_action->gobj(), g_variant_new_boolean(cross_probing_enabled));
				if(!cross_probing_enabled && !core_board.tool_is_active()) {
					highlights.clear();
					update_highlights();
				}
			});
		}

		add_tool_button(ToolID::MAP_PACKAGE, "Place package", false);

		{
			auto button = Gtk::manage(new Gtk::Button("Reload netlist"));
			button->signal_clicked().connect([this]{
				core_board.reload_netlist();
				canvas_update();
			});
			button->show();
			core.r->signal_tool_changed().connect([button](ToolID t){button->set_sensitive(t==ToolID::NONE);});
			main_window->header->pack_end(*button);
		}

		if(sockets_connected) {
			canvas->signal_selection_changed().connect(sigc::mem_fun(this, &ImpBoard::handle_selection_cross_probe));
		}


		fab_output_window = FabOutputWindow::create(main_window, core.b->get_board(), core.b->get_fab_output_settings());
		view_3d_window = View3DWindow::create(core_board.get_board(), pool.get());

		core.r->signal_tool_changed().connect([this](ToolID t){
			fab_output_window->set_can_generate(t==ToolID::NONE);
		});

		rules_window->signal_goto().connect([this] (Coordi location, UUID sheet) {
			canvas->center_and_zoom(location);
		});

		auto *display_control_notebook = Gtk::manage(new Gtk::Notebook);
		display_control_notebook->append_page(*layer_box, "Layers");
		layer_box->show();
		layer_box->get_style_context()->add_class("background");

		auto board_display_options = Gtk::manage(new BoardDisplayOptionsBox(core.b->get_layer_provider()));
		{
			auto fbox = Gtk::manage(new Gtk::Box());
			fbox->pack_start(*board_display_options, true, true, 0);
			fbox->get_style_context()->add_class("background");
			fbox->show();
			display_control_notebook->append_page(*fbox, "Options");
			board_display_options->show();
		}

		board_display_options->signal_set_layer_display().connect([this](int index, const LayerDisplay &lda){
			LayerDisplay ld = canvas->get_layer_display(index);
			ld.types_force_outline = lda.types_force_outline;
			ld.types_visible = lda.types_visible;
			canvas->set_layer_display(index, ld);
			canvas->queue_draw();
		});
		core.r->signal_rebuilt().connect([board_display_options] {
			board_display_options->update();
		});

		main_window->left_panel->pack_start(*display_control_notebook, false, false);
		display_control_notebook->show();

	}

	ToolID ImpBoard::handle_key(guint k) {
		return key_seq.handle_key(k);
	}
}
