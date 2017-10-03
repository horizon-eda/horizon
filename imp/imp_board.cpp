#include "imp_board.hpp"
#include "part.hpp"
#include "rules/rules_window.hpp"
#include "canvas/canvas_patch.hpp"

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
				{{GDK_KEY_X},				ToolID::ROUTE_TRACK},
				{{GDK_KEY_x},				ToolID::ROUTE_TRACK_INTERACTIVE},
				{{GDK_KEY_g},				ToolID::DRAG_KEEP_SLOPE},
		});
		key_seq.signal_update_hint().connect([this] (const std::string &s) {main_window->tool_hint_label->set_text(s);});

	}

	void ImpBoard::canvas_update() {
		canvas->update(*core_board.get_canvas_data());
		warnings_box->update(core_board.get_board()->warnings);
	}



	void ImpBoard::construct() {
		ImpLayer::construct();


		auto cam_job_button = Gtk::manage(new Gtk::Button("CAM Job"));
		main_window->top_panel->pack_start(*cam_job_button, false, false, 0);
		cam_job_button->show();
		cam_job_button->signal_clicked().connect([this]{cam_job_window->show_all();});
		core.r->signal_tool_changed().connect([cam_job_button](ToolID t){cam_job_button->set_sensitive(t==ToolID::NONE);});

		auto reload_netlist_button = Gtk::manage(new Gtk::Button("Reload netlist"));
		main_window->top_panel->pack_start(*reload_netlist_button, false, false, 0);
		reload_netlist_button->show();
		reload_netlist_button->signal_clicked().connect([this]{core_board.reload_netlist();canvas_update();});
		core.r->signal_tool_changed().connect([reload_netlist_button](ToolID t){reload_netlist_button->set_sensitive(t==ToolID::NONE);});

		auto test_button = Gtk::manage(new Gtk::Button("Test"));
		main_window->top_panel->pack_start(*test_button, false, false, 0);
		test_button->show();
		test_button->signal_clicked().connect([this] {
			CanvasPatch cp;
			cp.update(*core_board.get_board());

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


		});


		cam_job_window = CAMJobWindow::create(main_window, core.b);

		rules_window->signal_goto().connect([this] (Coordi location, UUID sheet) {
			canvas->center_and_zoom(location);
		});
	}

	ToolID ImpBoard::handle_key(guint k) {
		return key_seq.handle_key(k);
	}
}
