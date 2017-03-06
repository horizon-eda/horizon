#include "imp.hpp"
#include <gtkmm.h>
#include "block.hpp"
#include "core/core_board.hpp"
#include "core/tool_catalog.hpp"
#include "widgets/spin_button_dim.hpp"
#include "export_gerber/gerber_export.hpp"
#include "part.hpp"
#include "checks/checks_window.hpp"
#include "util.hpp"
#include <iomanip>
#include <glibmm/main.h>

namespace horizon {
	
	ImpBase::ImpBase(const std::string &pool_filename):
		pool(pool_filename),
		core(nullptr),
		sock_broadcast_rx(zctx, ZMQ_SUB),
		sock_project(zctx, ZMQ_REQ)
	{
		auto ep_broadcast = Glib::getenv("HORIZON_EP_BROADCAST");
		if(ep_broadcast.size()) {
			sock_broadcast_rx.connect(ep_broadcast);
			{
				unsigned int prefix = 0;
				sock_broadcast_rx.setsockopt(ZMQ_SUBSCRIBE, &prefix, 4);
				prefix = getpid();
				sock_broadcast_rx.setsockopt(ZMQ_SUBSCRIBE, &prefix, 4);
			}
			Glib::RefPtr<Glib::IOChannel> chan;
			#ifdef G_OS_WIN32
				SOCKET fd = sock_broadcast_rx.getsockopt<SOCKET>(ZMQ_FD);
				chan = Glib::IOChannel::create_from_win32_socket(fd);
			#else
				int fd = sock_broadcast_rx.getsockopt<int>(ZMQ_FD);
				chan = Glib::IOChannel::create_from_fd(fd);
			#endif

			Glib::signal_io().connect([this](Glib::IOCondition cond){
				while(sock_broadcast_rx.getsockopt<int>(ZMQ_EVENTS) & ZMQ_POLLIN) {
					zmq::message_t msg;
					sock_broadcast_rx.recv(&msg);
					int prefix;
					memcpy(&prefix, msg.data(), 4);
					char *data = ((char*)msg.data())+4;
					json j = json::parse(data);
					if(prefix == 0 || prefix==getpid()) {
						handle_broadcast(j);
					}

				}
				return true;
			}, chan, Glib::IO_IN | Glib::IO_HUP);
		}
		auto ep_project = Glib::getenv("HORIZON_EP_PROJECT");
		if(ep_project.size()) {
			sock_project.connect(ep_project);
		}
	}
	
	json ImpBase::send_json(const json &j) {
		std::string s = j.dump();
		zmq::message_t msg(s.size()+1);
		memcpy(((uint8_t*)msg.data()), s.c_str(), s.size());
		auto m = (char*)msg.data();
		m[msg.size()-1] = 0;
		sock_project.send(msg);

		zmq::message_t rx;
		sock_project.recv(&rx);
		char *rxdata = ((char*)rx.data());
		std::cout << "imp rx " << rxdata << std::endl;
		return json::parse(rxdata);
	}

	bool ImpBase::handle_close(GdkEventAny *ev) {
		bool dontask = false;
		Glib::getenv("HORIZON_NOEXITCONFIRM", dontask);
		if(dontask)
			return false;

		if(!core.r->get_needs_save())
			return false;

		Gtk::MessageDialog md(*main_window,  "Save changes before closing?", false /* use_markup */, Gtk::MESSAGE_QUESTION, Gtk::BUTTONS_NONE);
		md.set_secondary_text("If you don't save, all your changes will be permanently lost.");
		md.add_button("Close without Saving", 1);
		md.add_button("Cancel", Gtk::RESPONSE_CANCEL);
		md.add_button("Save", 2);
		switch(md.run()) {
			case 1:
				return false; //close

			case 2:
				core.r->save();
				return false; //close

			default:
				return true; //keep window open
		}
		return false;
	}

	void ImpBase::run(int argc, char *argv[]) {
		auto app = Gtk::Application::create(argc, argv, "net.carrotIndustries.horizon.Imp", Gio::APPLICATION_NON_UNIQUE);

		main_window = MainWindow::create();
		canvas = main_window->canvas;
		clipboard.reset(new ClipboardManager(core.r));

		canvas->signal_selection_changed().connect(sigc::mem_fun(this, &ImpBase::sc));
		canvas->signal_key_press_event().connect(sigc::mem_fun(this, &ImpBase::handle_key_press));
		canvas->signal_cursor_moved().connect(sigc::mem_fun(this, &ImpBase::handle_cursor_move));
		canvas->signal_button_press_event().connect(sigc::mem_fun(this, &ImpBase::handle_click));
		/*main_window->save_button->signal_clicked().connect(sigc::mem_fun(this, &ImpBase::handle_save));
		main_window->print_button->signal_clicked().connect(sigc::mem_fun(this, &ImpBase::handle_print));
		main_window->test_button->signal_clicked().connect(sigc::mem_fun(this, &ImpBase::handle_test));*/

		panels = Gtk::manage(new PropertyPanels(core.r));
		panels->show_all();
		main_window->property_viewport->add(*panels);
		panels->signal_update().connect(sigc::mem_fun(this, &ImpBase::canvas_update_from_pp));

		warnings_box = Gtk::manage(new WarningsBox());
		warnings_box->signal_selected().connect(sigc::mem_fun(this, &ImpBase::handle_warning_selected));
		main_window->left_panel->pack_end(*warnings_box, false, false, 0);

		tool_popover = Gtk::manage(new ToolPopover(canvas));
		tool_popover->set_position(Gtk::POS_BOTTOM);
		tool_popover->signal_tool_activated().connect([this](ToolID tool_id){
			ToolArgs args;
			args.coords = canvas->get_cursor_pos();
			args.selection = canvas->get_selection();
			args.work_layer = canvas->property_work_layer();
			ToolResponse r= core.r->tool_begin(tool_id, args, imp_interface.get());
			main_window->active_tool_label->set_text("Active tool: "+core.r->get_tool_name());
			tool_process(r);
		});

		selection_filter_dialog = std::make_unique<SelectionFilterDialog>(this->main_window, &canvas->selection_filter, core.r);

		key_sequence_dialog  = std::make_unique<KeySequenceDialog>(this->main_window);
		key_sequence_dialog->add_sequence(std::vector<unsigned int>{GDK_KEY_Page_Up}, "Layer up");
		key_sequence_dialog->add_sequence(std::vector<unsigned int>{GDK_KEY_Page_Down}, "Layer down");
		key_sequence_dialog->add_sequence(std::vector<unsigned int>{GDK_KEY_space}, "Begin tool");
		key_sequence_dialog->add_sequence(std::vector<unsigned int>{GDK_KEY_Home}, "Zoom all");
		key_sequence_dialog->add_sequence("Ctrl+Z", "Undo");
		key_sequence_dialog->add_sequence("Ctrl+Y", "Redo");
		key_sequence_dialog->add_sequence("Ctrl+C", "Copy");
		key_sequence_dialog->add_sequence("Ctrl+I", "Selection filter");
		key_sequence_dialog->add_sequence("Esc", "Hover select");


		check_runner = std::make_unique<CheckRunner>(core.r);


		grid_spin_button = Gtk::manage(new SpinButtonDim());
		grid_spin_button->set_range(0.1_mm, 10_mm);
		grid_spacing_binding = Glib::Binding::bind_property(grid_spin_button->property_value(), canvas->property_grid_spacing(), Glib::BINDING_BIDIRECTIONAL);
		grid_spin_button->set_value(1.25_mm);
		grid_spin_button->show_all();
		main_window->top_panel->pack_start(*grid_spin_button, false, false, 0);

		auto save_button = Gtk::manage(new Gtk::Button("Save"));
		save_button->signal_clicked().connect([this]{core.r->save();});
		save_button->show();
		main_window->top_panel->pack_start(*save_button, false, false, 0);

		auto selection_filter_button = Gtk::manage(new Gtk::Button("Selection filter"));
		selection_filter_button->signal_clicked().connect([this]{selection_filter_dialog->show();});
		selection_filter_button->show();
		main_window->top_panel->pack_start(*selection_filter_button, false, false, 0);

		auto help_button = Gtk::manage(new Gtk::Button("Help"));
		help_button->signal_clicked().connect([this]{key_sequence_dialog->show();});
		help_button->show();
		main_window->top_panel->pack_end(*help_button, false, false, 0);

		checks_window = ChecksWindow::create(main_window, check_runner.get(), canvas);

		{
			auto button = Gtk::manage(new Gtk::Button("Checks"));
			main_window->top_panel->pack_start(*button, false, false, 0);
			button->show();
			button->signal_clicked().connect([this]{checks_window->present();});
			core.r->signal_tool_changed().connect([button](ToolID t){button->set_sensitive(t==ToolID::NONE);});
		}

		core.r->signal_tool_changed().connect([save_button, selection_filter_button](ToolID t){save_button->set_sensitive(t==ToolID::NONE); selection_filter_button->set_sensitive(t==ToolID::NONE);});

		main_window->signal_delete_event().connect(sigc::mem_fun(this, &ImpBase::handle_close));

		construct();
		for(const auto &it: key_seq.get_sequences()) {
			key_sequence_dialog->add_sequence(it.keys, tool_catalog.at(it.tool_id).name);
		}


		imp_interface = std::make_unique<ImpInterface>(this);

		canvas_update();

		auto bbox = core.r->get_bbox();
		canvas->zoom_to_bbox(bbox.first, bbox.second);

		handle_cursor_move(Coordi()); //fixes label

		app->run(*main_window);
	}
	
	void ImpBase::canvas_update_from_pp() {
		auto sel = canvas->get_selection();
		canvas_update();
		canvas->set_selection(sel);
	}

	void ImpBase::tool_begin(ToolID id) {
		ToolArgs args;
		args.coords = canvas->get_cursor_pos();
		args.selection = canvas->get_selection();
		args.work_layer = canvas->property_work_layer();
		ToolResponse r= core.r->tool_begin(id, args, imp_interface.get());
		main_window->active_tool_label->set_text("Active tool: "+core.r->get_tool_name());
		tool_process(r);
	}

	void ImpBase::add_tool_button(ToolID id, const std::string &label) {
		auto button = Gtk::manage(new Gtk::Button(label));
		button->signal_clicked().connect([this, id]{
			tool_begin(id);
		});
		button->show();
		core.r->signal_tool_changed().connect([button](ToolID t){button->set_sensitive(t==ToolID::NONE);});
		main_window->top_panel->pack_start(*button, false, false, 0);
	}

	bool ImpBase::handle_key_press(GdkEventKey *key_event) {
		if(!core.r->tool_is_active()) {
			if(key_event->keyval == GDK_KEY_Escape) {
				canvas->selection_mode = CanvasGL::SelectionMode::HOVER;
				canvas->set_selection({});
				return true;
			}
			ToolID t = ToolID::NONE;
			if(!(key_event->state & Gdk::ModifierType::CONTROL_MASK)) {
				t = handle_key(key_event->keyval);
			}

			if(t != ToolID::NONE) {
				ToolArgs args;
				args.coords = canvas->get_cursor_pos();
				args.selection = canvas->get_selection();
				args.work_layer = canvas->property_work_layer();
				ToolResponse r= core.r->tool_begin(t, args, imp_interface.get());
				main_window->active_tool_label->set_text("Active tool: "+core.r->get_tool_name());
				tool_process(r);
				return true;
			}
			else {
				if((key_event->keyval == GDK_KEY_Page_Up) || (key_event->keyval == GDK_KEY_Page_Down)) {
					int wl = canvas->property_work_layer();
					auto layers = core.r->get_layers_sorted();
					int idx = std::find(layers.begin(), layers.end(), wl) - layers.begin();
					if(key_event->keyval == GDK_KEY_Page_Up) {
						idx++;
					}
					else {
						idx--;
					}
					if(idx>=0 && idx < (int)layers.size()) {
						canvas->property_work_layer() = layers.at(idx);
					}
					return true;
				}
				else if((key_event->keyval == GDK_KEY_space)) {
					Gdk::Rectangle rect;
					auto c = canvas->get_cursor_pos_win();
					rect.set_x(c.x);
					rect.set_y(c.y);
					tool_popover->set_pointing_to(rect);

					std::map<ToolID, bool> can_begin;
					auto sel = canvas->get_selection();
					for(const auto &it: tool_catalog) {
						bool r = core.r->tool_can_begin(it.first, sel);
						can_begin[it.first] = r;
					}
					tool_popover->set_can_begin(can_begin);

					#if GTK_CHECK_VERSION(3,22,0)
						tool_popover->popup();
					#else
						tool_popover->show();
					#endif
					return true;
				}
				else if((key_event->keyval == GDK_KEY_Home)) {
					auto bbox = core.r->get_bbox();
					canvas->zoom_to_bbox(bbox.first, bbox.second);
				}
				else if(key_event->keyval >= GDK_KEY_0 && key_event->keyval <= GDK_KEY_9) {
					int n = key_event->keyval - GDK_KEY_0;
					int layer = 0;
					if(n == 1) {
						layer = 0;
					}
					else if(n == 2) {
						layer = -100;
					}
					else {
						layer = -(n-2);
					}
					if(core.r->get_layers().count(layer)) {
						canvas->property_work_layer() = layer;
					}
				}
				else if(key_event->keyval == GDK_KEY_question) {
					key_sequence_dialog->show();
					return true;
				}

				if(key_event->state & Gdk::ModifierType::CONTROL_MASK) {
					if(key_event->keyval == GDK_KEY_z) {
						std::cout << "undo" << std::endl;
						core.r->undo();
						canvas_update_from_pp();
						return true;
					}
					else if(key_event->keyval == GDK_KEY_y) {
						std::cout << "redo" << std::endl;
						core.r->redo();
						canvas_update_from_pp();
						return true;
					}
					else if(key_event->keyval == GDK_KEY_c) {
						clipboard->copy(canvas->get_selection(), canvas->get_cursor_pos());
						return true;
					}
					else if(key_event->keyval == GDK_KEY_i) {
						selection_filter_dialog->show();
						return true;
					}
				}

			}
		}
		else {
			ToolArgs args;
			args.type = ToolEventType::KEY;
			args.coords = canvas->get_cursor_pos();
			args.key = key_event->keyval;
			args.work_layer = canvas->property_work_layer();
			ToolResponse r = core.r->tool_update(args);
			tool_process(r);
			return true;
		}
		return false;
	}
	
	void ImpBase::handle_cursor_move(const Coordi &pos) {
		if(core.r->tool_is_active()) {
			ToolArgs args;
			args.type = ToolEventType::MOVE;
			args.coords = pos;
			ToolResponse r = core.r->tool_update(args);
			tool_process(r);
		}
		main_window->cursor_label->set_text(coord_to_string(pos));
	}
	
	bool ImpBase::handle_click(GdkEventButton *button_event) {
		if(core.r->tool_is_active() && button_event->button != 2) {
			ToolArgs args;
			args.type = ToolEventType::CLICK;
			args.coords = canvas->get_cursor_pos();
			args.button = button_event->button;
			args.target = canvas->get_current_target();
			args.work_layer = canvas->property_work_layer();
			ToolResponse r = core.r->tool_update(args);
			tool_process(r);
		}
		return false;
	}

	void ImpBase::tool_process(const ToolResponse &resp) {
		if(!core.r->tool_is_active()) {
			main_window->active_tool_label->set_text("Active tool: None");
			main_window->tool_hint_label->set_text(">");

		}
		canvas_update();
		canvas->set_selection(core.r->selection);
		if(resp.layer != 10000) {
			canvas->property_work_layer() = resp.layer;
		}
		if(resp.next_tool != ToolID::NONE) {
			ToolArgs args;
			args.coords = canvas->get_cursor_pos();
			args.keep_selection = true;
			ToolResponse r = core.r->tool_begin(resp.next_tool, args, imp_interface.get());
			main_window->active_tool_label->set_text("Active tool: "+core.r->get_tool_name());
			tool_process(r);
		}
	}
	
	void ImpBase::sc(void) {
		//std::cout << "Selection changed\n";
		//std::cout << "---" << std::endl;
		if(!core.r->tool_is_active()) {
			auto sel = canvas->get_selection();
			decltype(sel) sel_extra;
			for(const auto &it: sel) {
				switch(it.type) {
					case ObjectType::SCHEMATIC_SYMBOL :
						sel_extra.emplace(core.c->get_schematic_symbol(it.uuid)->component->uuid, ObjectType::COMPONENT);
					break;
					case ObjectType::JUNCTION:
						if(core.r->get_junction(it.uuid)->net && core.c) {
							sel_extra.emplace(core.r->get_junction(it.uuid)->net->uuid, ObjectType::NET);
						}
					break;
					case ObjectType::LINE_NET: {
						LineNet &li = core.c->get_sheet()->net_lines.at(it.uuid);
						if(li.net) {
							sel_extra.emplace(li.net->uuid, ObjectType::NET);
						}
					}break;
					case ObjectType::NET_LABEL: {
						NetLabel &la = core.c->get_sheet()->net_labels.at(it.uuid);
						if(la.junction->net) {
							sel_extra.emplace(la.junction->net->uuid, ObjectType::NET);
						}
					}break;
					case ObjectType::POWER_SYMBOL: {
						PowerSymbol &sym = core.c->get_sheet()->power_symbols.at(it.uuid);
						if(sym.net) {
							sel_extra.emplace(sym.net->uuid, ObjectType::NET);
						}
					}break;
					case ObjectType::POLYGON_EDGE:
					case ObjectType::POLYGON_VERTEX: {
						sel_extra.emplace(it.uuid, ObjectType::POLYGON);
					}break;
					default: ;
				}
			}

			sel.insert(sel_extra.begin(), sel_extra.end());
			panels->update_objects(sel);
		}
		//for(const auto it: canvas->get_selection()) {
		//	std::cout << (std::string)it.uuid << std::endl;
		//}
		//std::cout << "---" << std::endl;
		std::set<UUID> net_segments;
		for(const auto it: canvas->get_selection()) {
			if(it.type == ObjectType::LINE_NET) {
				auto s_uuid = core.c->get_sheet()->net_lines.at(it.uuid).net_segment;
				assert(s_uuid);
				net_segments.insert(s_uuid);
			}
			if(it.type == ObjectType::TRACK) {
				auto s_uuid = core.b->get_board()->tracks.at(it.uuid).net_segment;
				net_segments.insert(s_uuid);
			}
			if(it.type == ObjectType::JUNCTION) {
				if(core.c) {
					auto s_uuid = core.c->get_sheet()->junctions.at(it.uuid).net_segment;
					net_segments.insert(s_uuid);
				}
				if(core.b) {
					auto s_uuid = core.b->get_board()->junctions.at(it.uuid).net_segment;
					net_segments.insert(s_uuid);
				}
			}
			if(it.type == ObjectType::POWER_SYMBOL) {
				if(core.c) {
					auto s_uuid = core.c->get_sheet()->power_symbols.at(it.uuid).junction->net_segment;
					net_segments.insert(s_uuid);
				}
			}
		}
		for(const auto &it :net_segments) {
			std::cout << "net seg " <<(std::string)it << std::endl;
		}
		//std::cout << "---" << std::endl;
	}

	void ImpBase::handle_tool_change(ToolID id) {
		panels->set_sensitive(id == ToolID::NONE);
		canvas->set_selection_allowed(id == ToolID::NONE);
		if(id!=ToolID::NONE) {
			main_window->tool_bar_set_tool_name(tool_catalog.at(id).name);
			main_window->tool_bar_set_tool_tip("");
		}
		main_window->tool_bar_set_visible(id!=ToolID::NONE);

	}

	void ImpBase::handle_warning_selected(const Coordi &pos) {
		canvas->center_and_zoom(pos);
	}

	bool ImpBase::handle_broadcast(const json &j) {
		std::string op = j.at("op");
		if(op == "present") {
			main_window->present();
			return true;
		}
		else if(op == "save") {
			core.r->save();
			return true;
		}
		return false;
	}

	void ImpBase::key_seq_append_default(KeySequence &ks) {
		ks.append_sequence({
				{{GDK_KEY_m},				ToolID::MOVE},
				{{GDK_KEY_M},				ToolID::MOVE_EXACTLY},
				{{GDK_KEY_Delete}, 			ToolID::DELETE},
				{{GDK_KEY_Return}, 			ToolID::ENTER_DATUM},
				{{GDK_KEY_r}, 				ToolID::ROTATE},
				{{GDK_KEY_e}, 				ToolID::MIRROR},
				{{GDK_KEY_Insert},			ToolID::PASTE},
		});
	}
}
